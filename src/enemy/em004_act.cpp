/*
 * enemy/em004_act.cpp - enemy 004's program: the program table `em004_prog_tbl` and its entries (init, frame update,
 *   action run, event notify, motion events, glow update, state query, death action, part checks, area-change check),
 *   the step functions of actions 0-3 and 7-13 with their `state_sub` dispatchers, the two sound-table players, the
 *   intro demo, the footstep effect dispatcher, the route-point pick and the static initializer, with the unit's
 *   turn/flight tables, key curves, route tables, sound tables and demo key tracks.
 * RANGE. .text 0x801C29F8-0x801CA8DC (107 functions); extab 0x8000FB84-0x8000FE4C, extabindex
 *   0x8002B830-0x8002BC5C, .ctors 0x8056F354-0x8056F358 (`em004_offset_init`), .rodata 0x80570320-0x80570410, .data
 *   0x805B3B80-0x805B4F8C (`em004_prog_tbl` first), .bss 0x806A7B18-0x806A7B30, .sdata 0x80791AD8-0x80791B20,
 *   .sdata2 0x80798FE8-0x80799220.  Left edge: `enemy/em006_prog.cpp`'s static initializer ends there; right
 *   edge: `em004_offset_init` is this TU's static initializer and the 0.0 entry repeats at 0x80799220 from
 *   0x801CA8DC on.  The `.data` order is the source order: the tables before the first function, the sound tables
 *   before their players, the demo tracks before the demo, the jump tables after their functions' predecessors.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi`
 *   pairs) and `#pragma pool_data off` (each table is addressed with its own `lis`/`addi`: pooled, the two sound
 *   players read 78); `#pragma fp_contract off` over `em004_act3_jump` (retail keeps `fmuls` + `fadds`).
 * NAMES. `em004_prog_tbl` is the runtime dump's name and opens the TU's `.data`; the `_act` suffix is a GUESS.
 *   Every other function, table and field name defined here is a GUESS from its body, its `em004_prog_tbl` slot,
 *   its dispatcher case or the data that points at it; `em004_act<N>_*` steps are named by the action id
 *   `em004_action_run` dispatches on.
 * RESIDUALS. Partial rows:
 *  - `em004_route_points_get` (8 B long): retail keeps the entry offset `i * 8` of the loop test in r5 across the
 *    match block, ours recomputes it (`clrlwi`/`slwi`) and colours `i`/`j` the other way round (the `break` and
 *    every loop/declaration spelling measured);
 *  - `em004_state_act_arm` (4 B short): retail branches over a `b` to the epilogue after `em_die_ck` (`bne`+`b`),
 *    ours folds it into one `beq` (early return, empty-then, success-first and `else return` all measured);
 *  - `em004_act8_call`: the max/targets and pending/bit register pairs swap (declaration hill-climb, local optimum);
 *  - `em004_act3_dive_turn2`: the turn flag's `and` result lands in r0, retail's in r7.
 *   Data: `.ctors` is not emitted (`em004_offset_init` is a plain function: `setVec3`/`assignVec3` are C-linkage
 *   helpers, not a constructor pair), so the unit cannot flip.
 * SHAPES. The `ShellSetFuncs` +0x14/+0x3C/+0x54/+0x64 calls go through spellings without the trailing table
 *   argument (the shared slot types hoist the table load).  Float arguments are listed where retail loads them:
 *   `em_ground_impact_set`/`em_gauge_period_step`/`em_gauge_tick` are called through `_v` cast views and
 *   `em_stamina_regen`/`eft013_spawn_id` are declared with the float before the trailing integers.  A narrowed ternary argument spells each constant with the cast
 *   (`map == 4 ? (u8)5 : (u8)0xFF`), and `em_mot_set`/`get_move_work_max` are called through their `u16`
 *   spellings, which keeps retail's `clrlwi`.  The motion-event switch lists `0xC` beside `0xCF`/`0xE5` and
 *   `0x66`/`0x67` for the frame-38 block (a decision-tree walk of retail's compares).  Two pool constants are
 *   computed in separate statements (`bias`, `dist`), which orders `.sdata2` like retail.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "sound/mhchar.h"
#include "sys_mem.h"
#include "fn_8004CAD8.h"
#include "mh3_pad.h"
#include "g3d/g3d_calcworld.h"
#include "draw_shape.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/em005_act.h"
#include "enemy/em004_act.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/fn_80138074.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_8011D448.h"
#include "enemy/enemy_control.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "stage/stg_w.h"
#include "stage/shell_set_func_ptr.h"
#include "sound/fn_800D7F54.h"
#include "sound/se.h"
#include "unsplit/ef.h"
#include "ef/eft009.h"
#include "ef/eft007.h"
#include "ef/eft019.h"
#include "ef/fn_80105314.h"
#include "ef/fn_8010D1A8.h"
#include "lobby/fn_8030121C.h"
#include "enemy/em_se_record.h"
#include "camera/camera_main.h"   /* camera_flag_0x145_set, camera_target_slot_clear */
#include "enemy/em_common.h"
#include "enemy/em_model.h"       /* em_drop_item_spawn */
#include "enemy/em_ground_impact_set.h"
#include "stage/stg_ground_impact_set.h"

/* Retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs the peephole folds. */
#pragma peephole off
#pragma pool_data off

struct EmFlyMotions;

/* the 8-byte `{key, count, subs}` lookup records `em004_route_points_get` walks (the pool at 0x805B3CD8). */
struct EmLookupSub {
    /* +0x0 */ u8 key;
    /* +0x1 */ u8 count;
    /* +0x2 */ u8 pad_0x02[2];
    /* +0x4 */ u8* points;
} /* size: 0x08 */;
struct EmLookupEntry {
    /* +0x0 */ u8 key;
    /* +0x1 */ u8 count;
    /* +0x2 */ u8 pad_0x02[2];
    /* +0x4 */ EmLookupSub* subs;
} /* size: 0x08 */;

/* The `ShellSetFuncs` slots spelled without their trailing table argument: the shared slot types hoist the
 * table load, retail loads it last (into the register the table argument would take). */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);
typedef void (*ShellPosJobFn)(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 a, u32 b, u16 c);
typedef void (*ShellJointJobFn)(struct _ENEMY_WORK* self, u32 a, u32 joint, f32 scale, u16 id);
typedef void (*ShellEftJobFn)(struct _ENEMY_WORK* self, u32 a, u32 b, nw4r::math::VEC3* pos);

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define em_fall_start_at ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
#define em_mot_set_motion ((void (*)(struct _ENEMY_WORK*, u16, s32, s32))em_mot_set)
#define get_move_work_count ((u16 (*)(u8))get_move_work_max)

/* Callees whose owner prototypes differ from the spelling retail's call sites were compiled with (the float listed
 * first, a narrowed or extra argument, a signed return): a cast call is the same direct call. */
#define em_ground_impact_set_v     ((void (*)(struct _ENEMY_WORK*, u8, u8, f32, nw4r::math::VEC3*))em_ground_impact_set)
#define em_gauge_tick_v ((void (*)(struct _ENEMY_WORK*, f32, s32, s16))em_gauge_tick)
#define em_gauge_period_step_v ((void (*)(struct _ENEMY_WORK*, f32, s32, s16))em_gauge_period_step)
#define em_act_arm_unless_act11_v ((void (*)(struct _ENEMY_WORK*, u8, u8))em_act_arm_unless_act11)
#define em_area_break_ck_v ((u8 (*)(struct _ENEMY_WORK*, u8))em_area_break_ck)
#define em_pack_area_pick_v ((u32 (*)(struct _ENEMY_WORK*, u8, u8, u16, u8*, u8*))em_pack_area_pick)
#define em_area_leave_ck_v ((s32 (*)(struct _ENEMY_WORK*))em_area_leave_ck)
#define em_stamina_add_v ((void (*)(struct _ENEMY_WORK*, s16))em_stamina_add)
#define stg_ground_impact_set_v ((void (*)(nw4r::math::VEC3*, u8, u16))stg_ground_impact_set)
#define em_move_rot_ground_step_v ((void (*)(struct _ENEMY_WORK*, u32*, f32))em_move_rot_ground_step)
#define em_motion_slot_angle_avg_v ((s32 (*)(struct _ENEMY_WORK*, u32))em_motion_slot_angle_avg)
#define em_turn_away_step_v ((void (*)(struct _ENEMY_WORK*, u32, u32))em_turn_away_step)
#define em_timer_0x81C_arm_v ((void (*)(struct _ENEMY_WORK*))em_timer_0x81C_arm)
#define em_team_mate_state_ck_v ((u32 (*)(struct _ENEMY_WORK*, u8, u8))em_team_mate_state_ck)


/* The unit's `.bss`: the two offset vectors the static initializer seeds.  The name is a GUESS. */
Vec em004_offset_vec[2];

extern "C" {

u32 em004_turn_away_start(struct _ENEMY_WORK* self, u16 motion, u8 kind);
void em004_init(struct _ENEMY_WORK* self, u8 mode);
void em004_idle(struct _ENEMY_WORK* self);
void em004_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub);
void em004_frame_update(struct _ENEMY_WORK* self);
void em004_action_run(struct _ENEMY_WORK* self);
void em004_motion_events(struct _ENEMY_WORK* self);
void em004_tev_color_update(struct _ENEMY_WORK* self);
u8 em004_state_query(struct _ENEMY_WORK* self, u8 kind);
void em004_death_action_get(struct _ENEMY_WORK* self, u8* out_mode, u8* out_flag);
void em004_noop(struct _ENEMY_WORK* self);
s32 em004_part_ck(struct _ENEMY_WORK* self, u8 kind);
void em004_part_break(struct _ENEMY_WORK* self, u8 part);
s32 em004_area_change_ck(struct _ENEMY_WORK* self, u16 timer);
void em004_rot_flip(struct _ENEMY_WORK* self);
void em004_idle_land(struct _ENEMY_WORK* self);
void em004_height_bias_set(struct _ENEMY_WORK* self);
void em004_hover_height_step(struct _ENEMY_WORK* self);
void em004_act0_motion1(struct _ENEMY_WORK* self);
void em004_act0_motion10(struct _ENEMY_WORK* self);
void em004_act0_fall(struct _ENEMY_WORK* self);
void em004_act0_ground(struct _ENEMY_WORK* self);
void em004_act0_dive(struct _ENEMY_WORK* self);
void em004_act1_slam(struct _ENEMY_WORK* self);
void em004_act1_roar(struct _ENEMY_WORK* self);
void em004_act1_motion1(struct _ENEMY_WORK* self);
void em004_act1_motion10(struct _ENEMY_WORK* self);
void em004_act1_breath(struct _ENEMY_WORK* self);
void em004_act1_motion2(struct _ENEMY_WORK* self);
void em004_act1_eat(struct _ENEMY_WORK* self);
void em004_act1_eat_end(struct _ENEMY_WORK* self);
void em004_act2_turn(struct _ENEMY_WORK* self);
void em004_act2_walk(struct _ENEMY_WORK* self, u8 mode);
void em004_act3_takeoff(struct _ENEMY_WORK* self);
void em004_act3_land(struct _ENEMY_WORK* self);
void em004_act3_climb(struct _ENEMY_WORK* self);
void em004_act3_hover_turn(struct _ENEMY_WORK* self);
void em004_act3_hover_wait(struct _ENEMY_WORK* self);
void em004_act3_fly_area(struct _ENEMY_WORK* self);
void em004_act3_hover_approach(struct _ENEMY_WORK* self);
void em004_act3_dive_attack(struct _ENEMY_WORK* self);
void em004_act3_ground_motion(struct _ENEMY_WORK* self, u8 mode);
void em004_act3_ground_turn(struct _ENEMY_WORK* self);
void em004_act3_ground_charge(struct _ENEMY_WORK* self);
void em004_act3_ground_pounce(struct _ENEMY_WORK* self);
void em004_act3_ground_motionD1(struct _ENEMY_WORK* self);
void em004_act3_ground_motionC9(struct _ENEMY_WORK* self);
void em004_act3_ground_slam(struct _ENEMY_WORK* self);
void em004_act3_ground_backstep(struct _ENEMY_WORK* self);
void em004_act3_ground_roar(struct _ENEMY_WORK* self);
void em004_act3_ground_motionD6(struct _ENEMY_WORK* self);
void em004_act3_ground_motion44(struct _ENEMY_WORK* self);
void em004_act3_ground_motion45(struct _ENEMY_WORK* self);
void em004_act3_jump(struct _ENEMY_WORK* self, u8 mode);
void em004_act3_dive_flip(struct _ENEMY_WORK* self, u8 mode);
void em004_act3_dive_turn(struct _ENEMY_WORK* self);
void em004_act3_dive_charge(struct _ENEMY_WORK* self);
void em004_act3_dive_back_leap(struct _ENEMY_WORK* self);
void em004_act3_dive_turn_leap(struct _ENEMY_WORK* self);
void em004_act3_dive_turn2(struct _ENEMY_WORK* self, u8 mode);
void em004_act3_short_jump(struct _ENEMY_WORK* self, u8 mode);
void em004_act3_dive_motionE8(struct _ENEMY_WORK* self);
void em004_act3_dive_motionE7(struct _ENEMY_WORK* self);
void em004_act3_dive_motionDE(struct _ENEMY_WORK* self);
void em004_act3_dive_slam(struct _ENEMY_WORK* self);
void em004_act3_dive_backstep(struct _ENEMY_WORK* self);
void em004_act3_dive_faint(struct _ENEMY_WORK* self);
void em004_act7_tail_swing(struct _ENEMY_WORK* self);
void em004_act7_charge_bite(struct _ENEMY_WORK* self);
void em004_act7_fire_breath(struct _ENEMY_WORK* self, u8 mode);
void em004_act7_flame_sweep(struct _ENEMY_WORK* self);
void em004_act7_flame_windup(struct _ENEMY_WORK* self);
void em004_act7_double_swing(struct _ENEMY_WORK* self);
void em004_act7_flame_pillar(struct _ENEMY_WORK* self);
void em004_act7_charge(struct _ENEMY_WORK* self, u8 mode);
void em004_act7_fire_spread(struct _ENEMY_WORK* self, u8 mode);
void em004_act7_ground_roar(struct _ENEMY_WORK* self);
void em004_act7_curve_charge(struct _ENEMY_WORK* self, u8 mode);
void em004_act7_jump2(struct _ENEMY_WORK* self);
void em004_act7_jump3(struct _ENEMY_WORK* self);
void em004_act7_dive_swing(struct _ENEMY_WORK* self);
void em004_act8_call(struct _ENEMY_WORK* self);
u32 em004_team_answer_ck(struct _ENEMY_WORK* self);
void em004_act9_call_loop(struct _ENEMY_WORK* self);
void em004_act9_call_heal(struct _ENEMY_WORK* self);
void em004_act9_call_end(struct _ENEMY_WORK* self);
void em004_act12_land(struct _ENEMY_WORK* self);
void em004_act13_demo(struct _ENEMY_WORK* self);
void em004_act13_demo_end(struct _ENEMY_WORK* self);
void em004_body_spark(struct _ENEMY_WORK* self);
void em004_step_eft_set(struct _ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, s32 id, f32 scale);
u32 em004_route_points_get(struct _ENEMY_WORK* self, u8 key, u8** out);
u8 em004_route_point_pick(struct _ENEMY_WORK* self);
void em004_spawn_rec_fill(EmSpawnRec* rec, u8 a, u16 b, u16 c);

/* ------------------------------------------------------------------------------------------------ */
/* the unit's data                                                                                   */
/* ------------------------------------------------------------------------------------------------ */

/* The `em_turn_seq_start`/`em_turn_seq_step` descriptor blocks (`.rodata`, 0x40 B each): a four-word head, then
 * three four-word rows whose first word carries the motion id in its high half. */
extern "C" const u32 em004_turn_tbl_walk[16] = {
    0x00000004, 0x00000020, 0x00000000, 0x40001555,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000C0000, 0x00000004, 0x00000000, 0x00000000,
    0x000D0000, 0x00000004, 0x00000000, 0x00000000,
};
extern "C" const u32 em004_turn_tbl_ground[16] = {
    0x00000004, 0x00000020, 0x00000000, 0x40001555,
    0x00D00000, 0x00000004, 0x00000000, 0x00000000,
    0x00CF0000, 0x00000004, 0x00000000, 0x00000000,
    0x00CE0000, 0x00000004, 0x00000000, 0x00000000,
};
extern "C" const u32 em004_turn_tbl_dive[16] = {
    0x00000004, 0x00000020, 0x00000000, 0x40001555,
    0x00E60000, 0x00000004, 0x00000000, 0x00000000,
    0x00E50000, 0x00000004, 0x00000000, 0x00000000,
    0x00E40000, 0x00000004, 0x00000000, 0x00000000,
};
/* The flight table `em_fly_start`/`em_fly_step` read (`enemy/em_common.cpp`'s `EmFlyMotions`: the glide and flap
 * motion requests, the two bank motions, then the maximum and minimum speed, 120.0 and 30.0). */
extern "C" const u32 em004_fly_tbl[12] = {
    0x00150000, 0x00000004, 0x00000000, 0x00000000,
    0x00160000, 0x0000000A, 0x00000000, 0x00000001,
    0x00170018, 0x42F00000, 0x41F00000, 0x00000000,
};

/* The em004 program table (`em004_prog_tbl`): the entry points the shared enemy program driver calls.  Slot
 * names are a GUESS from the functions they hold.
 * size: 0x70 */
struct Em004ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(struct _ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*idle_0x10)(struct _ENEMY_WORK* self);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ u8 (*state_query_0x24)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*noop_0x34)(struct _ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38[3];
    /* +0x44 */ s32 (*part_ck_0x44)(struct _ENEMY_WORK* self, u8 kind);
    /* +0x48 */ u32 pad_0x48[2];
    /* +0x50 */ void (*part_break_0x50)(struct _ENEMY_WORK* self, u8 part);
    /* +0x54 */ s32 (*area_change_ck_0x54)(struct _ENEMY_WORK* self, u16 timer);
    /* +0x58 */ u32 pad_0x58[6];
};

Em004ProgTbl em004_prog_tbl = {
    0,
    em004_init,
    em004_frame_update,
    em004_action_run,
    em004_idle,
    em004_event_notify,
    em004_motion_events,
    0,
    em004_tev_color_update,
    em004_state_query,
    {0, 0},
    em004_death_action_get,
    em004_noop,
    {0, 0, 0},
    em004_part_ck,
    {0, 0},
    em004_part_break,
    em004_area_change_ck,
    {0, 0, 0, 0, 0, 0},
};

/* The `em_key_curve_eval` curves (`{frame, value}` pairs, closed by a -1 frame): the take-off rise, the climb and
 * the pounce leap. */
f32 em004_takeoff_curve[14] = {
    0.0f, 57.0f, 8.0f, 51.0f, 16.0f, 22.0f, 24.0f, -5.0f, 32.0f, -13.0f, 48.0f, -12.0f, -1.0f, 0.0f,
};
f32 em004_climb_curve[20] = {
    0.0f, 14.0f, 10.0f, 21.6f, 20.0f, 18.0f, 30.0f, 1.2f, 50.0f, -1.2f,
    70.0f, 6.6f, 90.0f, 9.0f, 100.0f, -1.2f, 120.0f, 1.2f, -1.0f, 9.6f,
};
f32 em004_pounce_curve[12] = {
    0.0f, -8.5f, 8.0f, -14.6f, 11.0f, -26.0f, 15.0f, -40.4f, 22.0f, -47.7f, -1.0f, -47.7f,
};

/* The route-point lists of the nearest-point pick (`.sdata`): the points of one area's key. */
u8 em004_points_area4_key1[6] = {0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12};
u8 em004_points_area4_key2[2] = {0x13, 0x14};
u8 em004_points_area5_key3[3] = {0x08, 0x09, 0x0A};
u8 em004_points_area5_key4[3] = {0x0B, 0x0C, 0x0D};
u8 em004_points_area6_key1[4] = {0x06, 0x07, 0x08, 0x09};
u8 em004_points_area6_key2[4] = {0x0A, 0x0B, 0x0C, 0x0D};

/* The 30-frame ceiling run the dive attack arms. */
s16 em004_dive_frames = 30;

/* The per-area records of the route lookup and the table `em004_route_points_get` walks (closed by key 0xFF). */
EmLookupSub em004_route_subs_area4[2] = {
    {1, 6, {0, 0}, em004_points_area4_key1},
    {2, 2, {0, 0}, em004_points_area4_key2},
};
EmLookupSub em004_route_subs_area5[2] = {
    {3, 3, {0, 0}, em004_points_area5_key3},
    {4, 3, {0, 0}, em004_points_area5_key4},
};
EmLookupSub em004_route_subs_area6[2] = {
    {1, 4, {0, 0}, em004_points_area6_key1},
    {2, 4, {0, 0}, em004_points_area6_key2},
};
EmLookupEntry em004_route_tbl[4] = {
    {4, 2, {0, 0}, em004_route_subs_area4},
    {5, 2, {0, 0}, em004_route_subs_area5},
    {6, 2, {0, 0}, em004_route_subs_area6},
    {0xFF, 0, {0, 0}, 0},
};

/* The follow-area record the area-change check hands `em_pack_area_pick` (word-sized fields; their meaning is a
 * GUESS: a frame count, a packed id pair and an 0xFFFF terminator). */
u32 em004_follow_area_tbl[3] = {0x1E, 0x00C8000A, 0xFFFF0000};

/* 0x801C29F8 (0x60): averages the motion slots of `kind`; with a result, re-arms `motion` from it and returns 1. */
u32 em004_turn_away_start(struct _ENEMY_WORK* self, u16 motion, u8 kind) {
    s32 slot = em_motion_slot_angle_avg_v(self, kind);
    if (slot == -1) {
        return 0;
    }
    em_turn_away_step_v(self, motion, slot);
    return 1;
}

/* 0x801C2A58 (0x20): flips the halfword rotation and clears the third angle. */
void em004_rot_flip(struct _ENEMY_WORK* self) {
    self->field_0x1C0 = (s32)(u16)(self->field_0x1C0 + 0x8000);
    self->field_0x1C4 = 0;
}

/* 0x801C2A78 (0x110): while busy in the alive state, arms action 3's 0x18 or 0x2C step for the two map states. */
void em004_state_act_arm(struct _ENEMY_WORK* self) {
    if (em_busy_ck(self) != 0 && (u8)self->field_0x1E2 == 1 && (u8)self->action != 10) {
        if ((u32)em_die_ck(self) == 1) {
            return;
        }
        if ((u8)self->field_0x1E3 == 2) {
            if ((s32)self->field_0x23C == 0 && em_act_ck(self, 3, 0x18) == 0) {
                em_act_step_arm(self, 3, 0x18, 0);
                em_user_data_reenter(self);
                em_state_refresh(self);
            }
        } else if ((u8)self->field_0x1E3 == 1) {
            if (em_motion_slot_angle_avg_v(self, 0) == -1 && em_act_ck(self, 3, 0x2C) == 0) {
                em_act_step_arm(self, 3, 0x2C, 0);
                em_user_data_reenter(self);
                em_state_refresh(self);
            }
        }
    }
}

/* 0x801C2B88 (0x4C): drops to the floor and arms the idle action. */
void em004_idle_land(struct _ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    em_act_arm_unless_act11_v(self, 3, 4);
    em_finish_flag_set(self);
}

/* 0x801C2BD4 (0x64): derives the height bias from the size scale, clamped to [-100, 190]. */
void em004_height_bias_set(struct _ENEMY_WORK* self) {
    f32 bias = get_em_chg_scale(self) - 1.0f;
    bias = 1000.0f * bias;
    self->em004_0x328.height_bias_0x32C = bias;
    if (bias > 190.0f) {
        self->em004_0x328.height_bias_0x32C = 190.0f;
    } else if (bias < -100.0f) {
        self->em004_0x328.height_bias_0x32C = -100.0f;
    }
}

/* 0x801C2C38 (0xB8): raises the body by the ground clearance plus the height bias and keeps +0x1AC on its side. */
void em004_hover_height_step(struct _ENEMY_WORK* self) {
    f32 lift = 0.0f;
    f32 clear = self->pos.y - self->field_0x20C;
    if (clear < 1400.0f) {
        if (clear < 1200.0f) {
            clear = 1200.0f;
        }
        lift = 1400.0f - clear;
    }
    lift += self->em004_0x328.height_bias_0x32C;
    em_height_add(self, 0.05f * lift);
    if (lift < 0.0f) {
        if (self->field_0x1AC < lift) {
            self->field_0x1AC = lift;
        }
    } else if (self->field_0x1AC > lift) {
        self->field_0x1AC = lift;
    }
}

/* 0x801C2CF0 (0x118): the program's init: clears the +0x328 block, spawns the body glow and, in mode 2, places
 * the enemy on its map-4 area-4 route point. */
void em004_init(struct _ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    self->em004_0x328.fade_0x328 = 0.0f;
    self->em004_0x328.height_bias_0x32C = 0.0f;
    self->em004_0x328.timer_0x330 = 0;
    self->em004_0x328.count_0x332 = 0;
    self->em004_0x328.timer_0x334 = 0;
    em004_height_bias_set(self);
    if ((s32)self->field_0x009 == 0) {
        setVector3(&pos, 0.0f, 0.0f, 70.0f);
        eft013_spawn_id(self, 0xD, 0x1C, &pos, 1.0f, 0);
    }
    if ((s32)mode == 2) {
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 4 && (s32)self->area_no == 4) {
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
            self->pos.y += 5000.0f;
            em_fall_height_get(self);
            em_fall_start(self);
            em_act_arm_unless_down(self, 3, 1);
        }
        em_state_refresh(self);
    }
}

/* 0x801C2E08 (0x4): an empty body. */
void em004_idle(struct _ENEMY_WORK* self) {
}

/* 0x801C2E0C (0x248): the event notify: per `kind`/`sub`, flags, drop spawns and the 120-frame timer. */
void em004_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (kind) {
    case 1:
        switch (sub) {
        case 3:
            em_act_flag1_set(self);
            break;
        case 4:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x35, &pos);
                em_spawn_request(self->field_0x01A, 0x23, 1, self->area_no, 0xFF, 1, 3, 1, 0xFF, &pos,
                                 (s32)&self->field_0x1BC);
            }
            break;
        case 7:
            em_bait_react(self);
            break;
        }
        break;
    case 3:
        switch (sub) {
        case 5:
            self->field_0x1FB = 1;
            break;
        case 0x19:
        case 0x33:
            em_act_flag1_set(self);
            break;
        case 0x1F:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x35, &pos);
                em_spawn_request(self->field_0x01A, 0x23, 2, self->area_no, 0xFF, 1, 3, 1, 0xFF, &pos,
                                 (s32)&self->field_0x1BC);
            }
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 0x1D:
        case 0x23:
        case 0x35:
        case 0x36:
        case 0x58:
        case 0x5D:
        case 0x84:
        case 0x98:
        case 0x99:
        case 0x9E:
            self->timer_0x020 = 0x78;
            break;
        case 0x1B:
        case 0x5B:
            self->timer_0x020 = 0x78;
            break;
        }
        break;
    }
}

/* 0x801C3054 (0xB0): the per-frame timers: ticks +0x330/+0x334, counts +0x332 in the alive state, sets the
 * aim-target flag. */
void em004_frame_update(struct _ENEMY_WORK* self) {
    em004_state_act_arm(self);
    if (self->em004_0x328.timer_0x330 > 0) {
        self->em004_0x328.timer_0x330--;
    }
    if (self->em004_0x328.timer_0x334 > 0) {
        self->em004_0x328.timer_0x334--;
    }
    if ((u8)self->field_0x1E2 == 1 && (u8)self->field_0x1E3 == 2) {
        if (self->em004_0x328.count_0x332 < 300) {
            self->em004_0x328.count_0x332++;
        }
    } else if (self->em004_0x328.count_0x332 != 0) {
        self->em004_0x328.count_0x332 = 0;
    }
    if ((self->flags_0x836 & 0x8000) == 0) {
        self->flags_0x836 |= 0x8000;
    }
}

/* 0x801C3104 (0x7C): action 0: one motion, then finish. */
void em004_act0_motion1(struct _ENEMY_WORK* self) {
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

/* 0x801C3180 (0x7C): action 0 sub 2: one motion, then finish. */
void em004_act0_motion10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x10, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C31FC (0x80): action 0 sub 3: drops, one motion, then finish falling. */
void em004_act0_fall(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x11, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801C327C (0x8C): action 0 sub 8: the ground-mode motion, then the ground finish. */
void em004_act0_ground(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set_ck(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C3308 (0x8C): action 0 sub 9: the water-mode motion, then the water finish. */
void em004_act0_dive(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set_ck(self, 0xDE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C3394 (0x54): action 0's `state_sub` dispatcher. */
void em004_act0_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em004_act0_motion1(self);
        break;
    case 1:
        em004_act0_motion1(self);
        break;
    case 2:
        em004_act0_motion10(self);
        break;
    case 3:
        em004_act0_fall(self);
        break;
    case 8:
        em004_act0_ground(self);
        break;
    case 9:
        em004_act0_dive(self);
        break;
    }
}

/* 0x801C33E8 (0x17C): the ground slam: hit window, shape and dust at frame 114, dust trails from 120 to 240. */
void em004_act1_slam(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 4, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 8, 5);
            draw_shape_arm((u32)self, 0x1C, 0xA);
            stg_ground_impact_set_v(&self->pos, self->area_no, self->bits_0x1EC);
        }
        setVector3(&pos, 0.0f, 0.0f, 50.0f);
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 0x1C, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 120.0f, 240.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 0xD0, 0x1C, &pos, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C3564 (0x100): the stamina-costing roar: breath sparks every fourth frame from 20 to 68. */
void em004_act1_roar(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 4, 0);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 3, 20.0f, 68.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x80, 0x35, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 1, 4);
        }
        break;
    }
}

/* 0x801C3664 (0x7C): action 1 sub 2: one motion, then finish. */
void em004_act1_motion1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C36E0 (0x7C): action 1 sub 3: one motion, then finish. */
void em004_act1_motion10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C375C (0x118): the breath warm-up: one flash, then breath sparks every fourth frame from 40 to 88. */
void em004_act1_breath(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        setVector3(&pos, 0.0f, 0.0f, 0.0f);
        eft_em_spawn(self, 0x81, 0x35, &pos, 1.0f);
        break;
    case 1:
        if (em_frame_check(self, 3, 40.0f, 88.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x80, 0x35, &pos, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C3874 (0x7C): action 1 sub 5: one motion, then finish. */
void em004_act1_motion2(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C38F0 (0xC0): the meat eat: drops a meat, plays the motion twice, then action 1 sub 7. */
void em004_act1_eat(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE9, 4, 0);
        em_drop_item_spawn(self, &self->pos, 0x3C);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (++self->state_0x006 >= 2) {
                em_state_set(self, 1, 7);
            }
        }
        break;
    }
}

/* 0x801C39B0 (0x7C): action 1 sub 7: one motion, then finish. */
void em004_act1_eat_end(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xEA, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C3A2C (0x48): action 1's `state_sub` dispatcher. */
void em004_act1_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em004_act1_slam(self);
        break;
    case 1:
        em004_act1_roar(self);
        break;
    case 2:
        em004_act1_motion1(self);
        break;
    case 3:
        em004_act1_motion10(self);
        break;
    case 4:
        em004_act1_breath(self);
        break;
    case 5:
        em004_act1_motion2(self);
        break;
    case 6:
        em004_act1_eat(self);
        break;
    case 7:
        em004_act1_eat_end(self);
        break;
    }
}

/* 0x801C3A74 (0x8C): the turn sequence of the first table, then finish. */
void em004_act2_turn(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em004_turn_tbl_walk, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em004_turn_tbl_walk) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C3B00 (0xD4): the walk approach; mode 1 caps the approach distance at 700. */
void em004_act2_walk(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 6, 0);
        if ((s32)mode != 1) {
            em_approach_start(self, -100.0f, 0);
        } else {
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C3BD4 (0x38): action 2's `state_sub` dispatcher. */
void em004_act2_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1:
        em004_act2_turn(self);
        break;
    case 2:
        em004_act2_walk(self, 0);
        break;
    case 4:
        em004_act2_walk(self, 1);
        break;
    }
}

/* 0x801C3C0C (0x140): the take-off: lift motion, the key-curve climb, then the hover and finish falling. */
void em004_act3_takeoff(struct _ENEMY_WORK* self) {
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x14, 0, 0);
            em_move_vec_clr(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em004_takeoff_curve);
        em_move_rot_ground_step_v(self, &self->field_0x1BC, em_fall_height_get(self));
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x11, 6, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801C3D4C (0x12C): the landing: dive down to the ground, the landing motion, then finish. */
void em004_act3_land(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x11, 6, 0);
        em_dive_start(self);
        em_ceiling_latch(self);
        break;
    case 1:
        em_ceiling_latch(self);
        em_dive_step(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 6, 0);
        }
        break;
    case 2:
        em_ceiling_latch(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x19, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C3E78 (0x144): the climb: two key-curve rises, then lifts until the area ceiling. */
void em004_act3_climb(struct _ENEMY_WORK* self) {
    f32 limit;
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x11, 6, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = em_key_curve_eval(self, em004_climb_curve);
            em_move_offset_apply(self);
            if (em_mot_end_ck(self) == 1) {
                if (++self->state_0x007 >= 2) {

                    self->state_0x006++;
                    em_lift_start(self);
                }
            }
            break;
        case 1:
            em_lift_step(self);
            break;
        }
        limit = stg_area_ceiling_get(self->area_no) - 1000.0f;
        if (limit - self->field_0x20C < 2000.0f) {
            limit = 2000.0f + self->field_0x20C;
        }
        if (self->pos.y >= limit) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801C3FBC (0x90): the hover turn: turns to the target, then finish falling. */
void em004_act3_hover_turn(struct _ENEMY_WORK* self) {
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x11, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801C404C (0x98): the hover wait: 60 frames, then the idle action. */
void em004_act3_hover_wait(struct _ENEMY_WORK* self) {
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x15, 4, 0);
        self->timer_0x020 = 0x3C;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em004_idle_land(self);
        }
        break;
    }
}

/* 0x801C40E4 (0x104): the flight to the target area; on arrival idles there or moves to the next area. */
void em004_act3_fly_area(struct _ENEMY_WORK* self) {
    em_ceiling_latch(self);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fly_start(self, (struct EmFlyMotions*)em004_fly_tbl);
        em_approach_start(self, 0.0f, 0x19);
        em_move_rot_ground_step_v(self, &self->field_0x1BC, em_fall_height_get(self));
        break;
    case 1:
        if (em_approach_step(self, 0, 0) == 1) {
            if ((u8)self->area_no == self->field_0x9F8 || self->field_0x9F8 == 0xFF) {
                em004_idle_land(self);
            } else {
                em_state_set(self, 3, 5);
            }
            break;
        }
        em_fly_step(self, 0, (struct EmFlyMotions*)em004_fly_tbl);
        em_move_rot_ground_step_v(self, &self->field_0x1BC, em_fall_height_get(self));
        break;
    }
}

/* 0x801C41E8 (0xE0): the hover approach with the radius capped at 70. */
void em004_act3_hover_approach(struct _ENEMY_WORK* self) {
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x11, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 40.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_fall(self);
            break;
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 > 70.0f) {
            self->field_0x318 = 70.0f;
        }
        break;
    }
}

/* 0x801C42C8 (0x1C0): the dive attack: impact at frame 50, then the ceiling run until the timer ends. */
void em004_act3_dive_attack(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1B, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 50.0f);
            em_ground_impact_set_v(self, 2, 1, 2.5f, &pos);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            self->timer_0x020 = em004_dive_frames;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x1C, 0, 0);
            em_move_vec_clr(self);
            self->field_0x314 = 110.0f;
        }
        break;
    case 2:
        if ((s32)self->state_0x006 == 0) {
            em_move_offset_apply(self);
        }
        if (--self->timer_0x020 <= 0) {
            if (em_busy_ck(self) == 1) {
                em_action_finish_fall(self);
            }
        } else if (em_ground_snap_offset_ck(self, em_ceiling_depth_get(self)) == 1) {
            if ((s32)self->state_0x006 == 0) {
                self->state_0x006 = 1;
            }
            if (em_busy_ck(self) == 1) {
                em_state_set(self, 3, 0x15);
            }
        }
        break;
    }
}

/* 0x801C4488 (0xAC): the ground-mode motion (mode 1 picks the alternate), then the ground finish. */
void em004_act3_ground_motion(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set_motion(self, mode == 1 ? (u16)0xEE : (u16)0x1D, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C4534 (0x9C): the ground-mode turn sequence, then the ground finish. */
void em004_act3_ground_turn(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_turn_seq_start(self, (void*)em004_turn_tbl_ground, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em004_turn_tbl_ground) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C45D0 (0xF0): the ground-mode charge: approach under a hit window, the stop motion, then the ground finish. */
void em004_act3_ground_charge(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xCC, 6, 0);
        em_hit_window_set(self, 0, 1, 2);
        em_approach_start(self, -400.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0xCD, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C46C0 (0x28C): the ground-mode pounce: turn, leap toward the target clamped to [0, 1000], land with
 * an impact. */
void em004_act3_ground_pounce(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 pos;
    VEC3_ctor(&dir);
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xD3, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 3, 2.0f, 22.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            nw4r::math::VEC3 diff;
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_sub_mode_set(self, 0);
            em_mot_set(self, 0xD4, 0, 2);
            em_hit_window_set(self, 0, 5, 3);
            subVec3(&diff, &self->vec_0x36C, &self->pos);
            copyVec3(&dir, &diff);
            rotVecY(&dir, -self->field_0x1C0);
            if (dir.z < 0.0f) {
                dir.z = 0.0f;
            } else if (dir.z > 1000.0f) {
                dir.z = 1000.0f;
            }
            em_move_vec2_clr(self);
            self->field_0x318 = dir.z / 12.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em004_pounce_curve);
        CancelFade(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD5, 0, 0);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            setVector3(&pos, 0.0f, 0.0f, 50.0f);
            em_ground_impact_set_v(self, 2, 1, 2.5f, &pos);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C494C (0x8C): the ground-mode motion 0xD1, then the ground finish. */
void em004_act3_ground_motionD1(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set_ck(self, 0xD1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C49D8 (0x94): the ground-mode motion 0xC9 with a 90-frame timer, then the ground finish. */
void em004_act3_ground_motionC9(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xC9, 6, 0);
        self->timer_0x020 = 0x5A;
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C4A6C (0x18C): the ground-mode slam (the action-1 slam with the second dust trail). */
void em004_act3_ground_slam(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xCA, 4, 0);
        self->timer_0x020 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 8, 5);
            draw_shape_arm((u32)self, 0x1C, 0xA);
            stg_ground_impact_set_v(&self->pos, self->area_no, self->bits_0x1EC);
        }
        setVector3(&pos, 0.0f, 0.0f, 50.0f);
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 0x1C, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 120.0f, 240.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x1C, &pos, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C4BF8 (0xA4): the ground-mode backstep approach, then the ground finish. */
void em004_act3_ground_backstep(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xCB, 6, 0);
        em_approach_start(self, -200.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C4C9C (0xA0): the ground-mode roar, then action 3 sub 0x1F. */
void em004_act3_ground_roar(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xD2, 4, 0);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 3, 0x1F);
        }
        break;
    }
}

/* 0x801C4D3C (0x8C): the ground-mode motion 0xD6, then the ground finish. */
void em004_act3_ground_motionD6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0xD6, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C4DC8 (0x8C): the ground-mode motion 0x44, then the ground finish. */
void em004_act3_ground_motion44(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0x44, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C4E54 (0x8C): the ground-mode motion 0x45, then the ground finish. */
void em004_act3_ground_motion45(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0x45, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

#pragma fp_contract off
/* 0x801C4EE0 (0x3A0): the jump attack: modes 2/3 turn in the wind-up and aim the leap distance; mode 0 can
 * chain into the follow-up motion mid-air. */
void em004_act3_jump(struct _ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 pos2;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 saved;
    VEC3_ctor(&pos2);
    VEC3_ctor(&dir);
    VEC3_ctor(&saved);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1E, 4, 0);
        em_mot_speed_set(self, 1.8f);
        break;
    case 1:
        switch (mode) {
        case 2:
            if (em_frame_check(self, 3, 4.0f, 16.0f) == 1) {
                em_turn_to_target(self, 0x1D0);
            }
            break;
        case 3:
            if (em_frame_check(self, 3, 4.0f, 16.0f) == 1) {
                em_turn_to_target(self, 0x640);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x1F, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 70.0f;
            self->field_0x320 = -5.0f;
            self->field_0x318 = 60.0f;
            if ((u8)(mode + 0xFE) <= 1) {
                nw4r::math::VEC3 diff;
                f32 dist;
                subVec3(&diff, &self->vec_0x36C, &self->pos);
                copyVec3(&dir, &diff);
                rotVecY(&dir, -self->field_0x1C0);
                dist = dir.z - 1900.0f;
                if (dist > 0.0f) {
                    dist = 0.0f;
                } else if (dist < -800.0f) {
                    dist = -800.0f;
                }
                dist /= 10.0f;
                self->field_0x318 = self->field_0x318 + 0.25f * dist;
            }
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 2:
        copyVec3(&saved, &self->pos);
        CancelFade(self);
        em_turn_in_window(self, 0.0f, 36.0f, 0x8000);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x25, 0, 0);
            em_hit_window_set_default(self, 0, 0x11);
            break;
        }
        if ((s32)mode == 0 && em_busy_ck(self) == 1 && em_frame_check(self, 3, 18.0f, 40.0f) == 1 &&
            em004_turn_away_start(self, 0xFFFF, 1) == 1) {
            if (em_frame_check(self, 1, 28.0f, 0.0f) == 1) {
                em_state_set(self, 3, 0x31);
            } else {
                em_state_set(self, 3, 0x29);
            }
            copyVec3(&self->pos, &saved);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            setVector3(&pos2, 0.0f, 0.0f, -50.0f);
            em_ground_impact_set_v(self, 2, 1, 2.5f, &pos2);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

#pragma fp_contract reset

/* 0x801C5280 (0xDC): the water-mode back flip (mode 1 picks the alternate motion), then the water finish. */
void em004_act3_dive_flip(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0: {
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set_motion(self, mode == 1 ? (u16)0x24 : (u16)0x20, 0, 0);


        self->field_0x1C4 = 0x8000;
        em_move_vec_clr(self);
        self->field_0x318 = 450.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        em_move_offset_apply(self);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C535C (0x9C): the water-mode turn sequence, then the water finish. */
void em004_act3_dive_turn(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_turn_seq_start(self, (void*)em004_turn_tbl_dive, 0, 0x20, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em004_turn_tbl_dive) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C53F8 (0xF0): the water-mode charge: approach under a hit window, the stop motion, then the water finish. */
void em004_act3_dive_charge(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xE2, 6, 0);
        em_hit_window_set(self, 0, 1, 2);
        em_approach_start(self, -400.0f, 0x20);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0xE3, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C54E8 (0x1A4): the water-mode back leap: flip, leap at the target, land, then finish. */
void em004_act3_dive_back_leap(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xEF, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0xF0, 0, 0);
            em_hit_window_set(self, 0, 5, 3);
            em004_rot_flip(self);
            em_turn_to_target(self, 0xE39);
            em_move_vec2_clr(self);
            self->field_0x314 = -79.0f;
            self->field_0x318 = 133.3f;
            self->field_0x320 = 0.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 2:
        CancelFade(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xF1, 0, 0);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C568C (0x208): the water-mode turn leap: flip, leap, and from frame 14 chain into the ceiling run or the
 * pounce landing. */
void em004_act3_dive_turn_leap(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xEC, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0xED, 0, 0);
            em004_rot_flip(self);
            em_move_vec2_clr(self);
            self->field_0x314 = 54.0f;
            self->field_0x318 = 54.0f;
            self->field_0x320 = -3.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            self->timer_0x020 = 0;
        }
        break;
    case 2:
        CancelFade(self);
        self->timer_0x020++;
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD5, 0, 0);
            break;
        }
        if (em_busy_ck(self) == 1) {
            if (em_frame_check(self, 3, 14.0f, 40.0f) == 1) {
                if (em_ground_snap_offset_ck(self, em_ceiling_depth_get(self)) == 1) {
                    em_state_set(self, 3, 0x1A);
                }
            }
            if (em_frame_check(self, 0, 46.0f, 0.0f) == 1) {
                em_mot_set(self, 0xD4, 6, 0);
                em_hit_window_set(self, 0, 5, 3);
            }
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C5894 (0xC8): the water-mode turn sequence 2 (mode 1 turns the other way), then the water finish. */
void em004_act3_dive_turn2(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_turn_seq_start(self, (void*)em004_turn_tbl_dive, 2, 0x21, mode == 1 ? (u16)0x8000 : (u16)0);

        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em004_turn_tbl_dive) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C595C (0x288): the short jump: modes 1/3 use the alternate motions; modes 0/1 can chain into the
 * follow-up motion mid-air. */
void em004_act3_short_jump(struct _ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 pos2;
    nw4r::math::VEC3 saved;
    VEC3_ctor(&pos2);
    VEC3_ctor(&saved);
    switch (self->state) {
    case 0: {
        u16 motion;
        u32 blend;
        self->state++;
        em_move_mode_set(self, 0);
        if ((s32)mode != 1 && (s32)mode != 3) {
            motion = 0x22;
            blend = 6;
        } else {
            motion = 0x33;
            blend = 4;
        }
        em_mot_set(self, motion, blend, 0);
        break;
    }
    case 1:
        if (em_mot_end_ck(self) == 1) {
            u16 motion;
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            if ((s32)mode != 1 && (s32)mode != 3) {
                motion = 0x23;
            } else {
                motion = 0x35;
            }
            em_mot_set(self, motion, 0, 0);
            em_move_vec2_clr(self);
            self->field_0x314 = 70.0f;
            self->field_0x318 = -55.0f;
            self->field_0x320 = -6.2f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 2:
        copyVec3(&saved, &self->pos);
        CancelFade(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x25, 0, 0);
            em_hit_window_set_default(self, 0, 0x11);
            break;
        }
        if (mode <= 1 && em_busy_ck(self) == 1 && em_frame_check(self, 3, 10.0f, 30.0f) == 1 &&
            em004_turn_away_start(self, 0xFFFF, 1) == 1) {
            em_state_set(self, 3, 0x31);
            copyVec3(&self->pos, &saved);
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            setVector3(&pos2, 0.0f, 0.0f, -50.0f);
            em_ground_impact_set_v(self, 2, 1, 2.5f, &pos2);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C5BE4 (0x8C): the water-mode motion 0xE8, then the water finish. */
void em004_act3_dive_motionE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xE8, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C5C70 (0x8C): the water-mode motion 0xE7, then the water finish. */
void em004_act3_dive_motionE7(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set_ck(self, 0xE7, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C5CFC (0x8C): the water-mode motion 0xDE, then the water finish. */
void em004_act3_dive_motionDE(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xDE, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C5D88 (0x184): the water-mode slam (the action-1 slam in water mode). */
void em004_act3_dive_slam(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xE0, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 8, 5);
            draw_shape_arm((u32)self, 0x1C, 0xA);
            stg_ground_impact_set_v(&self->pos, self->area_no, self->bits_0x1EC);
        }
        setVector3(&pos, 0.0f, 0.0f, 50.0f);
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0, 0x1C, &pos, 1.0f);
        }
        if (em_frame_check(self, 3, 120.0f, 240.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                eft_em_spawn(self, 1, 0x1C, &pos, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C5F0C (0xA4): the water-mode backstep approach, then the water finish. */
void em004_act3_dive_backstep(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xE1, 6, 0);
        em_approach_start(self, -200.0f, 0x20);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C5FB0 (0x12C): the water-mode faint: knocked out for 1800 frames while regaining health, then wakes. */
void em004_act3_dive_faint(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0xEB, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xDD, 0xA, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick_v(self, 0.6f, 1, 0xA);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0xDF, 6, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C60DC (0xFC): action 3's `state_sub` dispatcher. */
void em004_act3_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x0:
        em004_act3_takeoff(self);
        break;
    case 0x1:
        em004_act3_land(self);
        break;
    case 0x2:
        em004_act3_climb(self);
        break;
    case 0x3:
        em004_act3_hover_turn(self);
        break;
    case 0x4:
        em004_act3_hover_wait(self);
        break;
    case 0x5:
        em004_act3_fly_area(self);
        break;
    case 0x6:
        em004_act3_short_jump(self, 2);
        break;
    case 0x7:
        em004_act3_short_jump(self, 3);
        break;
    case 0x8:
        em004_act3_jump(self, 1);
        break;
    case 0x9:
        em004_act3_hover_approach(self);
        break;
    case 0x14:
        em004_act3_dive_attack(self);
        break;
    case 0x15:
        em004_act3_ground_motion(self, 0);
        break;
    case 0x16:
        em004_act3_ground_turn(self);
        break;
    case 0x17:
        em004_act3_ground_charge(self);
        break;
    case 0x18:
        em004_act3_ground_pounce(self);
        break;
    case 0x19:
        em004_act3_ground_motionD1(self);
        break;
    case 0x1A:
        em004_act3_ground_motion(self, 1);
        break;
    case 0x1B:
        em004_act3_ground_motionC9(self);
        break;
    case 0x1C:
        em004_act3_ground_slam(self);
        break;
    case 0x1D:
        em004_act3_ground_backstep(self);
        break;
    case 0x1E:
        em004_act3_ground_roar(self);
        break;
    case 0x1F:
        em004_act3_ground_motionD6(self);
        break;
    case 0x20:
        em004_act3_ground_motion44(self);
        break;
    case 0x21:
        em004_act3_ground_motion45(self);
        break;
    case 0x28:
        em004_act3_jump(self, 0);
        break;
    case 0x29:
        em004_act3_dive_flip(self, 0);
        break;
    case 0x2A:
        em004_act3_dive_turn(self);
        break;
    case 0x2B:
        em004_act3_dive_charge(self);
        break;
    case 0x2C:
        em004_act3_dive_back_leap(self);
        break;
    case 0x2D:
        em004_act3_dive_turn_leap(self);
        break;
    case 0x2E:
        em004_act3_dive_turn2(self, 0);
        break;
    case 0x2F:
        em004_act3_dive_turn2(self, 1);
        break;
    case 0x30:
        em004_act3_short_jump(self, 0);
        break;
    case 0x31:
        em004_act3_dive_flip(self, 1);
        break;
    case 0x32:
        em004_act3_dive_motionE8(self);
        break;
    case 0x33:
        em004_act3_dive_motionE7(self);
        break;
    case 0x35:
        em004_act3_short_jump(self, 1);
        break;
    case 0x36:
        em004_act3_dive_motionDE(self);
        break;
    case 0x37:
        em004_act3_dive_slam(self);
        break;
    case 0x38:
        em004_act3_dive_backstep(self);
        break;
    case 0x39:
        em004_act3_dive_faint(self);
        break;
    }
}

/* 0x801C61D8 (0xF4): the tail swing: two hit windows, re-armed at frame 50. */
void em004_act7_tail_swing(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2F, 4, 0);
        em_hit_window_set(self, 0, 2, 8);
        em_hit_window_set(self, 1, 0x13, 0x18);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xD, 0x18);
            em_hit_window_set(self, 1, 0x14, 0x10);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C62CC (0x114): the charge bite: approach under a hit window, then the bite with two windows. */
void em004_act7_charge_bite(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 1, 0xA);
        em_approach_start(self, -800.0f, 0x10);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 0x30, 2, 0);
            em_hit_window_set(self, 0, 3, 0x18);
            em_hit_window_set(self, 1, 0x10, 0x10);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C63E0 (0x368): the fire breath: mode 0/1 shoot a fireball at frame 100 (mode 2 the part effect), mode 2
 * also scatters the shell job at frame 108. */
void em004_act7_fire_breath(struct _ENEMY_WORK* self, u8 mode) {
    EmSpawnRec rec;
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 offset;
    VEC3_ctor(&joint);
    VEC3_ctor(&offset);
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x31, 4, 0);
        em_hit_window_set_default(self, 0, (mode - 2 == 0) + 0x17);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 3, 4.0f, 44.0f) == 1) {
            em_turn_to_target(self, 0x200);
        }
        if (em_frame_check(self, 3, 12.0f, 60.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            eft007_part_set(self, 0xA);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            switch (mode) {
            default:
                em004_spawn_rec_fill(&rec, 0, 0x1000, 0);
                eft007_part_spawn(self, 0xB, rec.field_0x12, rec.field_0x14);
                if (em_parts_damage_level_get(self, 0) >= 2) {
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x11);
                } else {
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 8);
                }
                self->em004_0x328.timer_0x330 = 0x168;
                break;
            case 1:
                em004_spawn_rec_fill(&rec, 0, 0, 0);
                eft007_part_spawn(self, 0xB, rec.field_0x12, rec.field_0x14);
                if (em_parts_damage_level_get(self, 0) >= 2) {
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x11);
                } else {
                    ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 8);
                }
                self->em004_0x328.timer_0x330 = 0x168;
                break;
            case 2:
                eft007_part_spawn(self, 0x10, 0, 0);
                break;
            }
        }
        if (mode == 2 && em_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            offset.x = 0.0f;
            offset.y = 0.0f;
            offset.z = 100.0f;
            rotVecY(&offset, self->field_0x1C0);
            get_joint_wpos_em(self, 0x1C, &joint);
            addVec3To(&joint, &offset);
            if (em_parts_damage_level_get(self, 0) >= 2) {
                ((ShellPosJobFn)shell_set_func_ptr->method_0x14)(self, &joint, 0x1A, self->area_no, 0xFFFF);
            } else {
                ((ShellPosJobFn)shell_set_func_ptr->method_0x14)(self, &joint, 0x13, self->area_no, 0xFFFF);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C6748 (0x1E4): the flame sweep: a hit window at frame 176, alternating flame bursts every 12 frames,
 * then the closing motion at frame 264. */
void em004_act7_flame_sweep(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x32, 4, 0);
        em_stamina_add_v(self, -0xC);
        self->timer_0x020 = 0;
        self->state_0x006 = 0;
        break;
    case 1:
        if (em_frame_check(self, 0, 176.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, em_parts_damage_level_get(self, 2) >= 2 ? 0x16 : 4, 2);
        }
        if (em_after_frame_check(self, 1, 190.0f, 0.0f) == 1) {
            if (--self->timer_0x020 < 0) {
                setVector3(&pos, 0.0f, 0.0f, 0.0f);
                if ((s32)self->state_0x006 == 0) {
                    self->state_0x006 = 1;
                    eft_em_spawn_param(self, 0x1B, 3, &pos, 1.0f, 0);
                } else {
                    self->state_0x006 = 0;
                    eft_em_spawn_param(self, 0x1B, 3, &pos, 1.0f, 0x2000);
                }
                self->timer_0x020 = 0xC;
            }
        }
        if (em_frame_check(self, 0, 264.0f, 0.0f) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_mot_set(self, 0x34, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C692C (0xEC): the flame wind-up: part sparks every fourth frame from 20 to 68, then action 7 sub 8 at 114. */
void em004_act7_flame_windup(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 4, 0);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_frame_check(self, 3, 20.0f, 68.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            eft007_part_set(self, 0xC);
        }
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em_state_set(self, 7, 8);
        }
        break;
    }
}

/* 0x801C6A18 (0xE8): the double swing: two hit windows, then the closing motion. */
void em004_act7_double_swing(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x37, 4, 0);
        em_hit_window_set(self, 0, 0x15, 8);
        em_hit_window_set(self, 1, 0x19, 0x10);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x34, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C6B00 (0x1AC): the flame pillar: at frame 2 the core effect and the shell job at the head joint. */
void em004_act7_flame_pillar(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 0xA, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 0x35, &pos);
            pos.y = self->field_0x20C;
            if (em_parts_damage_level_get(self, 7) >= 2) {
                eft019_set_core(&pos, self->area_no, 0x5E, 1.0f, 1.0f);
                ((ShellJointJobFn)shell_set_func_ptr->method_0x64)(self, 1, 0x35, 1.0f, 0xFFFF);
            } else {
                eft019_set_core(&pos, self->area_no, 0x24, 1.0f, 1.0f);
                ((ShellJointJobFn)shell_set_func_ptr->method_0x64)(self, 0, 0x35, 1.0f, 0xFFFF);
            }
            self->em004_0x328.timer_0x334 = 0xD2;
        }
        if (em_frame_check(self, 3, 64.0f, 92.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            eft007_part_set(self, 0xC);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C6CAC (0x13C): the charge: approach (mode 1 capped at 700, mode 2 from -700), then the stop motion. */
void em004_act7_charge(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 1, 2);
        switch (mode) {
        default:
            em_approach_start(self, -100.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            if (self->value_0x378 > 700.0f) {
                self->value_0x378 = 700.0f;
            }
            break;
        case 2:
            em_approach_start(self, -700.0f, 0);
            break;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1) {
            self->state++;
            em_mot_set(self, 9, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C6DE8 (0x2C8): the ground-mode fire spread: turns while shooting three fireballs at frames 114, 244
 * and 374. */
void em004_act7_fire_spread(struct _ENEMY_WORK* self, u8 mode) {
    EmSpawnRec rec;
    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0x3C, 4, 0);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (mode == 1) {
            em_turn_in_window(self, 70.0f, 420.0f, -0x8000);
        } else {
            em_turn_in_window(self, 70.0f, 420.0f, 0x8000);
        }
        if (em_frame_check(self, 0, 114.0f, 0.0f) == 1) {
            em004_spawn_rec_fill(&rec, 0, 0x4000, 0);
            eft007_part_spawn(self, 0x1A, rec.field_0x12, rec.field_0x14);
            rec.pos.y = 30.0f;
            rec.pos.z = 60.0f;
            if (em_parts_damage_level_get(self, 0) >= 2) {
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x12);
            } else {
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 9);
            }
        }
        if (em_frame_check(self, 0, 244.0f, 0.0f) == 1) {
            em004_spawn_rec_fill(&rec, 0, 0x7000, 0);
            eft007_part_spawn(self, 0x1A, rec.field_0x12, rec.field_0x14);
            if (em_parts_damage_level_get(self, 0) >= 2) {
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x12);
            } else {
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 9);
            }
        }
        if (em_frame_check(self, 0, 374.0f, 0.0f) == 1) {
            em004_spawn_rec_fill(&rec, 0, 0x1000, 0);
            eft007_part_spawn(self, 0x1A, rec.field_0x12, rec.field_0x14);
            if (em_parts_damage_level_get(self, 0) >= 2) {
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 0x12);
            } else {
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 9);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C70B0 (0x1C4): the ground-mode roar: turns, then while busy targets the team mates it can call (action
 * 8) or falls back to action 3 sub 0x20. */
void em004_act7_ground_roar(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0x3F, 4, 0);
        em_hit_window_set_default(self, 0, 6);
        em_stamina_add_v(self, -0xC);
        em_height_set(self, 0.0f);
        break;
    case 1:
        em004_hover_height_step(self);
        switch (self->state_0x006) {
        case 0:
            if (em_frame_check(self, 3, 20.0f, 60.0f) == 1) {
                em_turn_to_target(self, 0x480);
            }
            if (em_mot_end_ck(self) != 1) {
                break;
            }
            self->state_0x006++;
            /* fall through */
        case 1:
            if (em_busy_ck(self) == 1) {
                u16 max = get_move_work_count(2);
                u8 i;
                u8 mask = 0;
                for (i = 0; i < max; i++) {
                    if (em_team_mate_state_ck_v(self, 0, i) == 1) {
                        mask |= (u8)(1 << i);
                    }
                }
                if (mask != 0) {
                    em_reaction_req(self, 1, 0xA, mask);
                    em_state_set(self, 8, 0);
                } else {
                    em_state_set(self, 3, 0x20);
                }
            }
            break;
        }
        break;
    }
}

/* 0x801C7274 (0x120): the curving charge: mode 1/2 bend the heading each frame. */
void em004_act7_curve_charge(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        em_hit_window_set(self, 0, 1, 2);
        em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        switch (mode) {
        case 1:
            self->field_0x1C0 = (s32)(u16)(self->field_0x1C0 + 0x80);
            break;
        case 2:
            self->field_0x1C0 = (s32)(u16)(self->field_0x1C0 - 0x80);
            break;
        }
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            em_mot_set(self, 9, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C7394 (0x8): the jump attack in mode 2. */
void em004_act7_jump2(struct _ENEMY_WORK* self) {
    em004_act3_jump(self, 2);
}

/* 0x801C739C (0x8): the jump attack in mode 3. */
void em004_act7_jump3(struct _ENEMY_WORK* self) {
    em004_act3_jump(self, 3);
}

/* 0x801C73A4 (0xC0): the water-mode double swing, then the water finish. */
void em004_act7_dive_swing(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 1);
        em_mot_set(self, 0x36, 4, 0);
        em_hit_window_set(self, 0, 0xE, 8);
        em_hit_window_set(self, 1, 0xF, 0x10);
        em_stamina_add_v(self, -0xC);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_dive(self);
        }
        break;
    }
}

/* 0x801C7464 (0xA8): action 7's `state_sub` dispatcher. */
void em004_act7_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em004_act7_tail_swing(self);
        break;
    case 1:
        em004_act7_charge_bite(self);
        break;
    case 2:
        em004_act7_fire_breath(self, 0);
        break;
    case 3:
        em004_act7_flame_sweep(self);
        break;
    case 4:
        em004_act7_fire_breath(self, 1);
        break;
    case 5:
        em004_act7_flame_windup(self);
        break;
    case 6:
        em004_act7_fire_breath(self, 2);
        break;
    case 7:
        em004_act7_double_swing(self);
        break;
    case 8:
        em004_act7_flame_pillar(self);
        break;
    case 9:
        em004_act7_charge(self, 0);
        break;
    case 10:
        em004_act7_fire_spread(self, 0);
        break;
    case 11:
        em004_act7_ground_roar(self);
        break;
    case 12:
        em004_act7_charge(self, 1);
        break;
    case 13:
        em004_act7_charge(self, 2);
        break;
    case 14:
        em004_act7_fire_spread(self, 1);
        break;
    case 15:
        em004_act7_curve_charge(self, 0);
        break;
    case 16:
        em004_act7_curve_charge(self, 1);
        break;
    case 17:
        em004_act7_curve_charge(self, 2);
        break;
    case 18:
        em004_act7_jump2(self);
        break;
    case 19:
        em004_act7_jump3(self);
        break;
    case 20:
        em004_act7_dive_swing(self);
        break;
    }
}

/* 0x801C750C (0x2B8): the call: roars, then picks the team mates in range of the head that answer (action 9)
 * or gives up after the timer. */
void em004_act8_call(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3 head;
    VEC3_ctor(&unused);
    VEC3_ctor(&head);
    em004_hover_height_step(self);
    self->flags_0x836 &= 0x7FFF;
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set_blend(self, 0x40, 4, 0, 1);
        em_guard_set(self);
        break;
    case 1:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 1) {
            em_timer_0x81C_arm_v(self);
        }
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em_camera_req(self, 0x1C, 1);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->timer_0x020 = 0x5A;
        }
        break;
    case 2:
        if (em_frame_check(self, 1, 50.0f, 0.0f) == 1) {
            em_timer_0x81C_arm_v(self);
        }
        if (self->timer_0x020 > 0) {
            self->timer_0x020--;
        }
        if (em_busy_ck(self) == 1) {
            s32 pending;
            u8* work;
            u8 targets;
            u8 i;
            u16 max;
            u8 found;
            f32 range;
            s32 bit;
            get_joint_wpos_em(self, 0x1C, &head);
            work = (u8*)get_move_work_adrs(2);
            max = get_move_work_max(2);
            targets = self->field_0x382;
            found = 0;
            pending = 0;
            i = 0;
            range = 25600.0f;
            for (; i < max; i++, work += 0xB20) {
                bit = 1 << i;
                if ((targets & bit) != 0 && *work != 0 &&
                    !(calcDistanceSqXZ((VEC3*)(work + 0x3C), (VEC3*)&head) > range)) {
                    if (em_team_mate_state_ck_v(self, 1, i) == 1) {
                        found |= bit;
                    } else if (em_team_mate_state_ck_v(self, 0, i) == 1) {
                        pending = 1;
                    }
                }
            }
            if (found != 0) {
                em_reaction_req(self, 1, 0xA, found);
                self->field_0x1E7 = 6;
                em_state_set(self, 9, 0);
            } else if (self->timer_0x020 <= 0 || pending == 0) {
                em_state_set(self, 3, 0x21);
            }
        }
        break;
    }
}

/* 0x801C77C4 (0x14): action 8's `state_sub` dispatcher. */
void em004_act8_run(struct _ENEMY_WORK* self) {
    if ((s32)self->state_sub == 0) {
        em004_act8_call(self);
    }
}

/* 0x801C77D8 (0x90): 1 when every team mate in the +0x382 mask still answers. */
u32 em004_team_answer_ck(struct _ENEMY_WORK* self) {
    u16 max;
    u8 i;
    u8 targets;
    max = get_move_work_count(2);
    targets = self->field_0x382;
    i = 0;
    for (; i < max; i++) {

        if ((targets & (1 << i)) != 0 && em_team_mate_state_ck_v(self, 1, i) == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x801C7868 (0x114): the call loop: while the team mates answer, counts the calls down (action 9 sub 2/1). */
void em004_act9_call_loop(struct _ENEMY_WORK* self) {
    em_timer_0x81C_arm_v(self);
    em004_hover_height_step(self);
    self->flags_0x836 &= 0x7FFF;
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0x42, 0, 0);
        em_guard_set(self);
        break;
    case 1:
        if (em_busy_ck(self) == 1) {
            if (em004_team_answer_ck(self) == 1) {
                if ((s32)--self->field_0x1E7 == 0) {
                    em_state_set(self, 9, 2);
                } else {
                    em_state_set(self, 9, 1);
                }
            } else {
                em_state_set(self, 3, 0x21);
            }
        }
        break;
    }
}

/* 0x801C797C (0x1AC): the call heal: at frame 32 regains health and stamina, then loops while the team mates
 * answer. */
void em004_act9_call_heal(struct _ENEMY_WORK* self) {
    em_timer_0x81C_arm_v(self);
    em004_hover_height_step(self);
    self->flags_0x836 &= 0x7FFF;
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set(self, 0x42, 0, 0);
        em_guard_set(self);
        em_hit_window_set_default(self, 0, 7);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            if (em_frame_check(self, 0, 32.0f, 0.0f) == 1) {
                em_gauge_period_step_v(self, 1.0f, (s32)(0.006f * (f32)(s32)self->field_0x7A4), 1);
                em_stamina_regen(self, 0.6f, 0x64, 1);
            }
            if (em_mot_end_ck(self) != 1) {
                break;
            }
            self->state_0x006++;
            em_mot_set(self, 0x42, 0, 0);
            /* fall through */
        case 1:
            if (em_busy_ck(self) == 1) {
                if (em004_team_answer_ck(self) == 1) {
                    em_state_set(self, 9, 0);
                } else {
                    em_state_set(self, 3, 0x21);
                }
            }
            break;
        }
        break;
    }
}

/* 0x801C7B28 (0x118): the call end: the closing roar with the camera shake, then the ground finish. */
void em004_act9_call_end(struct _ENEMY_WORK* self) {
    self->flags_0x836 &= 0x7FFF;
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_move_sub_mode_set(self, 2);
        em_mot_set_blend(self, 0x43, 8, 0, 1);
        em_hit_window_set_default(self, 0, 0x12);
        em_guard_set(self);
        em_timer_0x81C_arm_v(self);
        camera_flag_0x145_set();
        em004_hover_height_step(self);
        break;
    case 1:
        if (em_frame_check(self, 2, 198.0f, 0.0f) == 1) {
            em_timer_0x81C_arm_v(self);
            em004_hover_height_step(self);
        }
        if (em_frame_check(self, 0, 260.0f, 0.0f) == 1) {
            camera_target_slot_clear(0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_sub2(self);
        }
        break;
    }
}

/* 0x801C7C40 (0x30): action 9's `state_sub` dispatcher. */
void em004_act9_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em004_act9_call_loop(self);
        break;
    case 1:
        em004_act9_call_heal(self);
        break;
    case 2:
        em004_act9_call_end(self);
        break;
    }
}

/* The `.sdata` words the sound tables' 0xFE terminators point at (the loop record), and the two offsets
 * the shifted records carry. */
u32 em004_se_loop_mot1B = 0x0AEA0000;
u32 em004_se_loop_mot1D = 0x0AE40000;
u32 em004_se_loop_mot23[2] = {0x0AE40000, 0};
EmSeOffset em004_se_shift_tail = {-50.0f, -10.0f};
u32 em004_se_loop_mot35 = 0x0AE40000;
u32 em004_se_loop_mot9E = 0x0AE40000;
EmSeOffset em004_se_shift_head = {-5.0f, 60.0f};

/* The per-motion sound tables action 10 plays (`em_se_tbl_play`), each with its records. */
EmSeRecord em004_se_tbl_mot18_rec0 = {0x64, 0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot18[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot18_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot17_rec0 = {0x65, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot17[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot17_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot1E_rec0 = {0x6C, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot1E[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot1E_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot1B_rec0 = {0x66, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot1B[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot1B_rec0},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em004_se_loop_mot1B},
};
EmSeRecord em004_se_tbl_mot1D_rec0 = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot1D[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot1D_rec0},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em004_se_loop_mot1D},
};
EmSeRecord em004_se_tbl_mot23_rec0 = {0x70, 0, 2, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em004_se_tbl_mot23_rec1 = {0x0, 0, 0, 0, 0, {0, 2, 1, 0}, (const EmSeOffset*)&em004_se_shift_tail};
EmSeRecord em004_se_tbl_mot23_rec2 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot23[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x2C)},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot23_rec0},
    {0x01, {0, 0, 0}, &em004_se_tbl_mot23_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot23_rec2},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em004_se_loop_mot23},
};
EmSeRecord em004_se_tbl_mot35_rec0 = {0x70, 0, 0, 56, 0, {0, 2, 1, 0}, (const EmSeOffset*)&em004_se_shift_tail};
EmSeRecord em004_se_tbl_mot35_rec1 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot35[4] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot35_rec0},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot35_rec1},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em004_se_loop_mot35},
};
EmSeRecord em004_se_tbl_mot37_rec0 = {0x41, 0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot37[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot37_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot9E_rec0 = {0x41, 0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_tbl_mot9E_rec1 = {0x70, 0, 0, 56, 0, {0, 2, 1, 0}, (const EmSeOffset*)&em004_se_shift_tail};
EmSeRecord em004_se_tbl_mot9E_rec2 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot9E[6] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot9E_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot9E_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot9E_rec2},
    {0xFE, {0, 0, 0}, (const EmSeRecord*)&em004_se_loop_mot9E},
};
EmSeRecord em004_se_tbl_mot7A_rec0 = {0x74, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot7A[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7A_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot7B_rec0 = {0x75, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot7B[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7B_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot7C_rec0 = {0x73, 0, 6, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot7C[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7C_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot8D_rec0 = {0x7C, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot8D[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot8D_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot7E_rec0 = {0x66, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_tbl_mot7E_rec1 = {0x68, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em004_se_tbl_mot7E_rec2 = {0x6A, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot7E[4] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7E_rec0},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7E_rec1},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7E_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot7F_rec0 = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_tbl_mot7F_rec1 = {0x69, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em004_se_tbl_mot7F_rec2 = {0x6B, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot7F[4] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7F_rec0},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7F_rec1},
    {0x00, {0, 0, 0}, &em004_se_tbl_mot7F_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_mot9F_rec0 = {0x6B, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_mot9F[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_mot9F_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motA5_rec0 = {0xDF, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motA5[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motA5_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motB6_rec0 = {0x7F, 0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motB6[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motB6_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motB7_rec0 = {0x80, 0, 10, 0, 0, {2, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motB7[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motB7_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motB8_rec0 = {0x14, 0, 4, 4, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em004_offset_vec};
EmSeRecord em004_se_tbl_motB8_rec1 = {0x11, 0, 6, 0, 0, {0, 3, 4, 0}, (const EmSeOffset*)&em004_se_shift_head};
EmSeRecord em004_se_tbl_motB8_rec2 = {0x12, 0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_tbl_motB8_rec3 = {0x19, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motB8[8] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_tbl_motB8_rec0},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1E0)},
    {0x00, {0, 0, 0}, &em004_se_tbl_motB8_rec1},
    {0x00, {0, 0, 0}, &em004_se_tbl_motB8_rec2},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_tbl_motB8_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motB9_rec0 = {0x82, 0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motB9[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motB9_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motBA_rec0 = {0x82, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motBA[2] = {
    {0x01, {0, 0, 0}, &em004_se_tbl_motBA_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motBB_rec0 = {0x82, 0, 10, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motBB[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motBB_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motBF_rec0 = {0x65, 0, 4, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em004_se_tbl_motBF_rec1 = {0x73, 0, 12, 0, 0, {2, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motBF[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x34)},
    {0x00, {0, 0, 0}, &em004_se_tbl_motBF_rec0},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_tbl_motBF_rec1},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motE4_rec0 = {0x69, 0, 4, 0, 0, {0, 4, 0, 0}, 0};
EmSeEntry em004_se_tbl_motE4[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motE4_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motE5_rec0 = {0x6B, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motE5[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motE5_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motE6_rec0 = {0x72, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motE6[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motE6_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motE7_rec0 = {0x72, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motE7[2] = {
    {0x01, {0, 0, 0}, &em004_se_tbl_motE7_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motE8_rec0 = {0x81, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motE8[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motE8_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motE9_rec0 = {0x69, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motE9[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motE9_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motEF_rec0 = {0x68, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em004_se_tbl_motEF[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motEF_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motEA_rec0 = {0x68, 0, 4, 0, 0, {0, 4, 0, 0}, 0};
EmSeEntry em004_se_tbl_motEA[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motEA_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em004_se_tbl_motEB_rec0 = {0x6A, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_tbl_motEB[2] = {
    {0x00, {0, 0, 0}, &em004_se_tbl_motEB_rec0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801C7C70 (0x43C): action 10: plays the motion's sound table. */
void em004_act10_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17: em_se_tbl_play(self, em004_se_tbl_mot17, 0, 0x17); break;
    case 0x18: em_se_tbl_play(self, em004_se_tbl_mot18, 0, 0x18); break;
    case 0x19: em_se_tbl_play(self, em004_se_tbl_mot18, 0, 0x19); break;
    case 0x1A: em_se_tbl_play(self, em004_se_tbl_mot18, 0, 0x1A); break;
    case 0x1B: em_se_tbl_play(self, em004_se_tbl_mot1B, 0, 0x1B); break;
    case 0x1C: em_se_tbl_play(self, em004_se_tbl_mot18, 0, 0x1C); break;
    case 0x1D: em_se_tbl_play(self, em004_se_tbl_mot1D, 0, 0x1D); break;
    case 0x1E: em_se_tbl_play(self, em004_se_tbl_mot1E, 0, 0x1E); break;
    case 0x58: em_se_tbl_play(self, em004_se_tbl_mot1D, 0, 0x58); break;
    case 0x5B: em_se_tbl_play(self, em004_se_tbl_mot1B, 0, 0x5B); break;
    case 0x5D: em_se_tbl_play(self, em004_se_tbl_mot1D, 0, 0x5D); break;
    case 0x23: em_se_tbl_play(self, em004_se_tbl_mot23, 1, 0x23); break;
    case 0x35: em_se_tbl_play(self, em004_se_tbl_mot35, 1, 0x35); break;
    case 0x36: em_se_tbl_play(self, em004_se_tbl_mot23, 1, 0x36); break;
    case 0x37: em_se_tbl_play(self, em004_se_tbl_mot37, 0xFF, 0x37); break;
    case 0x78: em_se_tbl_play(self, em004_se_tbl_mot17, 0, 0x78); break;
    case 0x7A: em_se_tbl_play(self, em004_se_tbl_mot7A, 0, 0x7A); break;
    case 0x7B: em_se_tbl_play(self, em004_se_tbl_mot7B, 0, 0x7B); break;
    case 0x7C: em_se_tbl_play(self, em004_se_tbl_mot7C, 0, 0x7C); break;
    case 0x8D: em_se_tbl_play(self, em004_se_tbl_mot8D, 0, 0x8D); break;
    case 0x7E: em_se_tbl_play(self, em004_se_tbl_mot7E, 0, 0x7E); break;
    case 0x7F: em_se_tbl_play(self, em004_se_tbl_mot7F, 0, 0x7F); break;
    case 0x8E: em_se_tbl_play(self, em004_se_tbl_mot8D, 0, 0x8E); break;
    case 0x84: em_se_tbl_play(self, em004_se_tbl_mot23, 1, 0x84); break;
    case 0x98: em_se_tbl_play(self, em004_se_tbl_mot23, 1, 0x98); break;
    case 0x99: em_se_tbl_play(self, em004_se_tbl_mot23, 1, 0x99); break;
    case 0x9E: em_se_tbl_play(self, em004_se_tbl_mot9E, 0xFF, 0x9E); break;
    case 0x9F: em_se_tbl_play(self, em004_se_tbl_mot9F, 0, 0x9F); break;
    case 0xA0: em_se_tbl_play(self, em004_se_tbl_mot9F, 0, 0xA0); break;
    case 0xA5: em_se_tbl_play(self, em004_se_tbl_motA5, 0xFE, 0xA5); break;
    case 0xB6: em_se_tbl_play(self, em004_se_tbl_motB6, 0, 0xB6); break;
    case 0xB7: em_se_tbl_play(self, em004_se_tbl_motB7, 0, 0xB7); break;
    case 0xB8: em_se_tbl_play(self, em004_se_tbl_motB8, 1, 0xB8); break;
    case 0xB9: em_se_tbl_play(self, em004_se_tbl_motB9, 0, 0xB9); break;
    case 0xBA: em_se_tbl_play(self, em004_se_tbl_motBA, 0, 0xBA); break;
    case 0xBB: em_se_tbl_play(self, em004_se_tbl_motBB, 0, 0xBB); break;
    case 0xBC: em_se_tbl_play(self, em004_se_tbl_motBB, 0, 0xBC); break;
    case 0xBF: em_se_tbl_play(self, em004_se_tbl_motBF, 0, 0xBF); break;
    case 0xC1: em_se_tbl_play(self, em004_se_tbl_mot8D, 0, 0xC1); break;
    case 0xE4: em_se_tbl_play(self, em004_se_tbl_motE4, 0, 0xE4); break;
    case 0xE5: em_se_tbl_play(self, em004_se_tbl_motE5, 0, 0xE5); break;
    case 0xE6: em_se_tbl_play(self, em004_se_tbl_motE6, 0, 0xE6); break;
    case 0xE7: em_se_tbl_play(self, em004_se_tbl_motE7, 0, 0xE7); break;
    case 0xE8: em_se_tbl_play(self, em004_se_tbl_motE8, 0, 0xE8); break;
    case 0xE9: em_se_tbl_play(self, em004_se_tbl_motE9, 0, 0xE9); break;
    case 0xEA: em_se_tbl_play(self, em004_se_tbl_motEA, 0, 0xEA); break;
    case 0xEB: em_se_tbl_play(self, em004_se_tbl_motEB, 0, 0xEB); break;
    case 0xEC: em_se_tbl_play(self, em004_se_tbl_motE6, 0, 0xEC); break;
    case 0xED: em_se_tbl_play(self, em004_se_tbl_motE7, 0, 0xED); break;
    case 0xEE: em_se_tbl_play(self, em004_se_tbl_motE8, 0, 0xEE); break;
    case 0xEF: em_se_tbl_play(self, em004_se_tbl_motEF, 0, 0xEF); break;
    case 0xF0: em_se_tbl_play(self, em004_se_tbl_mot37, 0xFF, 0xF0); break;
    default: em_action_finish(self); break;
    }
}

/* The per-motion sound tables action 11 plays (`em_se_tbl_play_alt`). */
EmSeRecord em004_se_alt_tbl_mot00_rec0 = {0x7D, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot00[2] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot00_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot05_rec0 = {0x70, 0, 2, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em004_se_alt_tbl_mot05_rec1 = {0x0, 0, 0, 0, 0, {0, 2, 1, 0}, (const EmSeOffset*)&em004_se_shift_tail};
EmSeRecord em004_se_alt_tbl_mot05_rec2 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_alt_tbl_mot05_rec3 = {0x72, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot05[7] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1C)},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot05_rec0},
    {0x01, {0, 0, 0}, &em004_se_alt_tbl_mot05_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot05_rec2},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot05_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot0C_rec0 = {0x70, 0, 0, 56, 0, {0, 2, 1, 0}, (const EmSeOffset*)&em004_se_shift_tail};
EmSeRecord em004_se_alt_tbl_mot0C_rec1 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_alt_tbl_mot0C_rec2 = {0x72, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot0C[5] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0C_rec0},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0C_rec1},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0C_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot0E_rec0 = {0x41, 0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_alt_tbl_mot0E_rec1 = {0x70, 0, 0, 56, 0, {0, 2, 1, 0}, (const EmSeOffset*)&em004_se_shift_tail};
EmSeRecord em004_se_alt_tbl_mot0E_rec2 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_alt_tbl_mot0E_rec3 = {0x72, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot0E[7] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0E_rec0},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1)},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0E_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0E_rec2},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0E_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot0F_rec0 = {0x82, 0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot0F[3] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot0F_rec0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot1C_rec0 = {0x82, 0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot1C[2] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot1C_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot1A_rec0 = {0x74, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em004_se_alt_tbl_mot1A_rec1 = {0x75, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot1A[3] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot1A_rec0},
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot1A_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em004_se_alt_tbl_mot34_rec0 = {0x72, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em004_se_alt_tbl_mot34[3] = {
    {0x00, {0, 0, 0}, &em004_se_alt_tbl_mot34_rec0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801C80AC (0x100): action 11: plays the motion's alternate sound table. */
void em004_act11_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x0: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot00, 0, 0); break;
    case 0x5: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot05, 1, 5); break;
    case 0xC: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot0C, 1, 0xC); break;
    case 0xD: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot05, 1, 0xD); break;
    case 0xE: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot0E, 0xFF, 0xE); break;
    case 0xF: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot0F, 0, 0xF); break;
    case 0x1A: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot1A, 0, 0x1A); break;
    case 0x1C: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot1C, 0, 0x1C); break;
    case 0x34: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot34, 0, 0x34); break;
    case 0x35: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot34, 0, 0x35); break;
    default: em_se_tbl_play_alt(self, em004_se_alt_tbl_mot00, 0, 0); break;
    }
}

/* 0x801C81AC (0x4): the landing step (a tail call). */
void em004_act12_land(struct _ENEMY_WORK* self) {
    em004_act3_land(self);
}

/* 0x801C81B0 (0x14): action 12's `state_sub` dispatcher. */
void em004_act12_run(struct _ENEMY_WORK* self) {
    if ((s32)self->state_sub == 0) {
        em004_act12_land(self);
    }
}

/* The intro demo's key tracks (`em_demo_key_apply`): the `{frame, x, z}` track of the walk, the
 * `{frame, x, y, z}` track of the leap and its `{frame, angle}` turn, each closed by a -1 frame. */
f32 em004_demo_walk_keys[66] = {
    0.0f, -255.67f, -2485.78f,
    4.0f, -257.17f, -2486.21f,
    8.0f, -261.16f, -2487.34f,
    12.0f, -266.9f, -2488.96f,
    16.0f, -273.63f, -2490.87f,
    20.0f, -280.64f, -2492.85f,
    24.0f, -287.19f, -2494.7f,
    28.0f, -292.53f, -2496.22f,
    32.0f, -295.91f, -2497.17f,
    36.0f, -296.52f, -2497.34f,
    40.0f, -293.54f, -2496.5f,
    44.0f, -282.81f, -2493.46f,
    48.0f, -264.42f, -2488.26f,
    52.0f, -242.1f, -2481.94f,
    56.0f, -218.22f, -2475.19f,
    60.0f, -194.41f, -2468.45f,
    64.0f, -171.87f, -2462.07f,
    68.0f, -151.52f, -2456.31f,
    72.0f, -134.13f, -2451.39f,
    74.0f, -126.75f, -2449.3f,
    76.0f, -207.02f, -2137.37f,
    -1.0f, -207.02f, -2137.37f,
};
f32 em004_demo_leap_keys[44] = {
    76.0f, -207.02f, 807.8f, -2137.37f,
    78.0f, -205.59f, 807.37f, -2135.36f,
    82.0f, -226.26f, 799.51f, -2130.03f,
    86.0f, -273.13f, 771.68f, -2124.04f,
    90.0f, -340.29f, 712.12f, -2117.54f,
    94.0f, -423.74f, 618.78f, -2110.48f,
    98.0f, -523.7f, 510.39f, -2103.56f,
    102.0f, -642.69f, 393.58f, -2097.65f,
    106.0f, -768.35f, 268.38f, -2089.8f,
    108.0f, -796.29f, 231.86f, -2086.92f,
    -1.0f, -796.29f, 231.86f, -2086.92f,
};
f32 em004_demo_turn_keys[24] = {
    76.0f, -90.0f,
    78.0f, -88.44f,
    80.0f, -84.55f,
    82.0f, -79.08f,
    84.0f, -72.46f,
    88.0f, -56.77f,
    92.0f, -38.88f,
    96.0f, -19.62f,
    100.0f, 0.29f,
    104.0f, 20.0f,
    108.0f, 20.0f,
    -1.0f, 20.0f,
};

/* 0x801C81C4 (0x634): the intro demo: the timed motion, camera position and rotation keys. */
void em004_act13_demo(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x48) == 1) {
            self->state++;
            em_demo_enable(self);
            em_fall_start_at(self, 0.0f);
            em_mot_set(self, 0xCC, 0, 0x46);
            em_demo_pos_set(self, 1007.335f, 1522.6617f, -2913.5522f);
            em_demo_rot_set(self, 0.0f, -7.5f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(0xD6) == 1) {
            self->state++;
            em_demo_reset(self, 0);
        }
        break;
    case 3:
        if (em_demo_time_ck(0x134) == 1) {
            self->state++;
            em_mot_set(self, 8, 0, 0x60);
            em_move_mode_set(self, 0);
            em_demo_enable(self);
            em_demo_pos_set(self, 427.35931f, 7.9003f, -465.4141f);
            em_demo_rot_set(self, 0.0f, -134.75f, 0.0f);
        }
        break;
    case 4:
        if (em_demo_time_ck(0x178) == 1) {
            self->state++;
            em_mot_set(self, 0xDE, 0, 0x5E);
            em_fall_start_at(self, 0.0f);
            em_demo_pos_set(self, -588.87128f, 867.82312f, -2503.7019f);
            em_demo_rot_set(self, -6.0f, 181.3f, 0.0f);
        }
        break;
    case 5:
        if (em_demo_time_ck(0x2D2) == 1) {
            self->state++;
            em_mot_set(self, 0xD3, 0, 0);
            self->timer_0x020 = 0;
            em_demo_pos_set(self, -255.67f, 807.8f, -2485.78f);
            em_demo_key_apply(self, self->timer_0x020, em004_demo_walk_keys, 0, 5, 0);
            em_demo_rot_set(self, -90.0f, -15.8f, 0.0f);
            em_hit_arm_set(self);
        }
        break;
    case 6:
        em_demo_key_apply(self, self->timer_0x020, em004_demo_walk_keys, 0, 5, 0);
        em_hit_arm_set(self);
        if (em_demo_time_ck(0x31E) == 1) {
            self->state++;
            em_mot_set(self, 0xD4, 0, 0);
        }
        break;
    case 7:
        em_demo_key_apply(self, self->timer_0x020, em004_demo_leap_keys, em004_demo_turn_keys, 7, 1);
        if (em_demo_time_ck(0x340) == 1) {
            self->state++;
            em_mot_set(self, 0xD5, 0, 8);
            em_move_mode_set(self, 0);
            em_demo_pos_set(self, -57.010502f, 1.4f, -1597.74f);
        }
        break;
    case 8:
        if (em_demo_time_ck(0x38E) == 1) {
            self->state++;
            em_mot_set(self, 1, 6, 0);
        }
        break;
    case 9:
        if (em_demo_time_ck(0x3AA) == 1) {
            self->state++;
            em_mot_set(self, 0x1A, 0, 0);
            em_demo_pos_set(self, -605.28607f, 1.4f, -975.67352f);
        }
        break;
    case 10:
        if (em_frame_check(self, 3, 20.0f, 68.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x80, 0x35, &pos, 1.0f);
        }
        if (em_demo_time_ck(0x430) == 1) {
            self->state++;
            em_mot_set(self, 0x1A, 0, 0x72);
            em_demo_pos_set(self, -447.64239f, 1.4f, -1413.7517f);
            em_demo_rot_set(self, 0.0f, 9.75f, 0.0f);
        }
        break;
    case 11:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x21, 0, 0);
        }
        break;
    case 12:
        if (em_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            eft_em_spawn(self, 0x81, 0x35, 0, 1.0f);
        }
        if (em_frame_check(self, 3, 40.0f, 88.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&pos, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x80, 0x35, &pos, 1.0f);
        }
        if (em_demo_time_ck(0x52C) == 1) {
            self->state++;
            em_mot_set(self, 8, 0, 0x1E);
            em_demo_pos_set(self, -421.48691f, 1.4f, -1261.5358f);
            em_demo_rot_set(self, 0.0f, 9.75f, 0.0f);
        }
        break;
    case 13:
        if (em_demo_time_ck(0x77A) == 1) {
            self->state++;
            em_mot_set(self, 1, 0, 0xBD);
            em_demo_pos_set(self, -605.77777f, 1.4f, -802.10101f);
            em_demo_rot_set(self, 0.0f, 165.0f, 0.0f);
        }
        break;
    }
    self->timer_0x020++;
}

/* 0x801C87F8 (0xA4): the demo's end pose, then finish. */
void em004_act13_demo_end(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_pos_set(self, -605.77777f, 1.4f, -802.10101f);
        em_demo_rot_set(self, 0.0f, 165.0f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801C889C (0x24): action 13's `state_sub` dispatcher. */
void em004_act13_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em004_act13_demo(self);
        break;
    case 1:
        em004_act13_demo_end(self);
        break;
    }
}

/* 0x801C88C0 (0x54): the action dispatcher. */
void em004_action_run(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em004_act0_run(self);
        break;
    case 1:
        em004_act1_run(self);
        break;
    case 2:
        em004_act2_run(self);
        break;
    case 3:
        em004_act3_run(self);
        break;
    case 7:
        em004_act7_run(self);
        break;
    case 8:
        em004_act8_run(self);
        break;
    case 9:
        em004_act9_run(self);
        break;
    case 10:
        em004_act10_se_play(self);
        break;
    case 11:
        em004_act11_se_play(self);
        break;
    case 12:
        em004_act12_run(self);
        break;
    case 13:
        em004_act13_run(self);
        break;
    }
}

/* 0x801C8914 (0x84): in the alternate mode, a body spark every 32 frames. */
void em004_body_spark(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) == 1 && (system_w.field_0x0c & 0x1F) == 0) {
        setVector3(&pos, 0.0f, 0.0f, 50.0f);
        eft_spawn_type10(self, 0x17, 0x1C, &pos, 1.0f);
    }
}

/* 0x801C8998 (0x3A0): the footstep effect dispatcher: per effect kind, `eft009_set_pos` at the body when the joint
 * is 0xFF, else at the joint; modes 1/2 the area and splash variants. */
void em004_step_eft_set(struct _ENEMY_WORK* self, u8 mode, u8 kind, u32 joint, s32 id, f32 scale) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (mode) {
    case 0:
        if ((self->field_0x228 & 0x6) != 0) {
            switch (kind) {
            case 2:
            case 4:
                kind = 0xC;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xE, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, id, scale);
                }
                break;
            case 5:
                kind = 0x11;
                break;
            case 8:
                kind = 0xC;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xE, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xE, id, scale);
                }
                break;
            case 0x1A:
                kind = 0x1B;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                }
                break;
            case 0x28:
                kind = 0xD;
                if (joint == 0xFF) {
                    pos.x = self->pos.x;
                    pos.y = 5.0f + self->field_0x210;
                    pos.z = self->pos.z;
                    eft009_set_pos(0xF, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, joint, 0xF, id, scale);
                }
                break;
            case 0:
            case 1:
            case 3:
            case 6:
            case 7:
            case 9:
            case 0xA:
            case 0xB:
            case 0x24:
            case 0x25:
            case 0x29:
            case 0x2A:
                return;
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
            case 0x28:
            case 0x2A:
            case 0x2B:
                kind = 2;
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
            eft009_spawn_at_joint(self, joint, kind, id, scale);
        }
        break;
    case 1:
        if ((self->field_0x228 & 0x6) == 0) {
            if (joint == 0xFF) {
                copyVec3(&pos, &self->pos);
            } else {
                get_joint_wpos_em(self, joint, &pos);
            }
            pos.y = self->field_0x20C;
            scale *= get_em_chg_scale(self);
            eft_spawn_pos_in_area(&pos, self->area_no, kind, id, scale);
        }
        break;
    case 2:
        if (joint == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        if (self->pos.y <= 900.0f + self->field_0x20C) {
            eft_spawn_type11(self, &pos, kind, scale);
        }
        break;
    }
}

/* 0x801C8D38 (0x12CC): the motion events: footstep and impact effects, hit windows and camera shakes per motion
 * frame, and the alternate-mode part flag. */
void em004_motion_events(struct _ENEMY_WORK* self) {
    EmSpawnRec rec;
    nw4r::math::VEC3 sp8;

    VEC3_ctor(&sp8);
    em_spawn_rec_init(&rec);
    em004_body_spark(self);
    switch (em_get_mot_no(self)) {
    case 0x5:
        if ((em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 292.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 0, 2, 0x11, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 318.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 0, 2, 8, 0, 1.0f);
        }
        break;
    case 0x7:
        if ((em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 0, 2, 8, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 2, 0x11, 0, 0.8f);
        }
        break;
    case 0x8:
        if ((em_after_frame_check(self, 0, 40, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 0, 0x28, 8, 0, 1.0f);
        }
        if ((em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 0, 0x28, 0x24, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 70, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x11, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 98.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x2C, 0, 1.0f);
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 4, 0x11, 0, 1.0f);
        }
        break;
    case 0xC:
    case 0xCF:
    case 0xE5:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 9, 8);
            em_hit_window_set(self, 1, 0xA, 0x10);
        }
        break;
    case 0xE4:
    case 0xCE:
    case 0xD:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xB, 8);
            em_hit_window_set(self, 1, 0xC, 0x10);
        }
        break;
    case 0xE:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 4, 8, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 4, 0x11, 0, 0.8f);
        }
        break;
    case 0x11:
        if ((em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 2, 0, 0xFF, 0, 0.6f);
        }
        break;
    case 0x12:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 2, 0, 0xFF, 0, 0.6f);
        }
        break;
    case 0x14:
        if (em_after_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 2, 0, 0xFF, 0, 0.6f);
        }
        break;
    case 0x19:
        if (em_after_frame_check(self, 0, 50, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x15, 0xFF, 0, 1.3f);
            } else {
                em004_step_eft_set(self, 1, 4, 0xFF, 0, 0.6f);
            }
            em_camera_req(self, (u32)-1, 7);
        }
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x14, 0xFF, 0, 1.3f);
            } else {
                em004_step_eft_set(self, 1, 4, 0xFF, 0, 0.6f);
            }
        }
        break;
    case 0x1B:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 5, 0xFF, 0, 0.8f);
        }
        break;
    case 0x1E:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 5, 0xFF, 0, 0.7f);
        }
        break;
    case 0x22:
        if (em_after_frame_check(self, 0, 50, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x15, 0xFF, 0, 1.3f);
            } else {
                em004_step_eft_set(self, 0, 5, 0xFF, 0, 0.7f);
            }
        }
        break;
    case 0x30:
        if ((em_after_frame_check(self, 0, 10, 0.0f) == 1) || (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1)) {
            em004_step_eft_set(self, 0, 0x28, 8, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 16.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x24, 0, 0.6f);
        }
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x11, 0, 1.0f);
        }
        if (em_after_frame_check(self, 0, 50, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x2C, 0, 0.6f);
        }
        break;
    case 0x32:
        if ((em_after_frame_check(self, 3, 140, 164.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp8, 0, 0, 0.0f);
            eft_em_spawn(self, 0x7A, 3, &sp8, 1.0f);
        }
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            setVector3(&sp8, 0, 0, 0.0f);
            eft_em_spawn(self, 0x79, 3, &sp8, 1.0f);
        }
        break;
    case 0x33:
        if (em_after_frame_check(self, 0, 62.0f, 0.0f) == 1) {
            setVector3(&sp8, 0, 0, 180.0f);
            if (em_parts_damage_level_get(self, 0) >= 2) {
                ((ShellEftJobFn)shell_set_func_ptr->method_0x54)(self, 3, 0x1C, &sp8);
            } else {
                ((ShellEftJobFn)shell_set_func_ptr->method_0x54)(self, 1, 0x1C, &sp8);
            }
            setVector3(&sp8, 0, 0, 300.0f);
            eft_em_spawn(self, 0x7B, 0x1C, &sp8, 1.0f);
        }
        break;
    case 0x37:
        if (em_after_frame_check(self, 0, 110, 0.0f) == 1) {
            em004_step_eft_set(self, 1, 4, 3, 0, 0.7f);
            em_camera_req(self, (u32)-1, 7);
        }
        break;
    case 0x43:
        if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            setVector3(&sp8, 0, 0, 0.0f);
            eft_em_spawn(self, 0x36, 0x1C, &sp8, 1.0f);
        }
        if (em_after_frame_check(self, 0, 96.0f, 0.0f) == 1) {
            em_camera_req(self, 0x1C, 1);
        }
        if (em_after_frame_check(self, 0, 120, 0.0f) == 1) {
            setVector3(&sp8, 0, 0, 0.0f);
            eft_em_spawn(self, 0x26, 0x1C, &sp8, 1.0f);
        }
        if ((em_after_frame_check(self, 3, 204.0f, 230.0f) == 1) && ((system_w.field_0x0c & 3) == 0)) {
            setVector3(&sp8, 0, 0, 80.0f);
            eft_em_spawn(self, 0x48, 0x1C, &sp8, 1.0f);
        }
        break;
    case 0x64:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 2, 8, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 40, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 2, 0x11, 0, 0.8f);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, 30, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x24, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x2C, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x15, 0xFF, 0, 1.3f);
                em004_step_eft_set(self, 0, 0xE, 0xFF, 0, 3.0f);
            } else {
                em004_step_eft_set(self, 1, 4, 0xFF, 0, 0.6f);
            }
        }
        break;
    case 0x66:
    case 0x67:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x11, 0xFF, 0, 1.0f);
            } else {
                em004_step_eft_set(self, 1, 0, 0xFF, 0, 1.0f);
            }
            em_camera_req(self, (u32)-1, 7);
        }
        break;
    case 0x6A:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 2, 0x11, 0, 1.2f);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 2, 8, 0, 1.2f);
        }
        break;
    case 0x6C:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x24, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 40, 0.0f) == 1) {
            em004_step_eft_set(self, 0, 0x28, 0x2C, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x15, 0xFF, 0, 1.0f);
                em004_step_eft_set(self, 0, 0xE, 0xFF, 0, 2.5f);
            } else {
                em004_step_eft_set(self, 1, 4, 0xFF, 0, 0.7f);
            }
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x11, 0xFF, 0, 0.9f);
            } else {
                em004_step_eft_set(self, 1, 0, 0xFF, 0, 0.9f);
            }
            em_camera_req(self, (u32)-1, 1);
        }
        break;
    case 0x74:
        if (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x11, 0xFF, 0, 0.7f);
            } else {
                em004_step_eft_set(self, 1, 0, 0xFF, 0, 1.0f);
            }
            em_camera_req(self, (u32)-1, 7);
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x11, 0xFF, 0, 1.2f);
            } else {
                em004_step_eft_set(self, 1, 5, 0xFF, 0, 0.6f);
            }
        }
        if (em_after_frame_check(self, 0, 218.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em004_step_eft_set(self, 0, 0x10, 0xFF, 0, 1.0f);
            } else {
                em004_step_eft_set(self, 1, 1, 0xFF, 0, 0.9f);
            }
            em_camera_req(self, (u32)-1, 7);
        }
        if (em_after_frame_check(self, 0, 400, 0.0f) == 1) {
            em004_step_eft_set(self, 1, 1, 0x1A, 0, 0.6f);
        }
        break;
    case 0x7F:
        if (em_after_frame_check(self, 0, 20, 0.0f) == 1) {
            em004_step_eft_set(self, 1, 2, 0xFF, 0, 0.7f);
            em_camera_req(self, (u32)-1, 7);
        }
        break;
    case 0x82:
        if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 1, 1, 0xFF, 0, 0.8f);
            em_camera_req(self, (u32)-1, 7);
        }
        break;
    case 0xD5:
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &sp8);
            sp8.y = self->field_0x20C;
            eft_spawn_pos_in_area(&sp8, self->area_no, 4, self->field_0x1C0, get_em_chg_scale(self));
        }
        break;
    case 0xF1:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em004_step_eft_set(self, 1, 4, 0xFF, 0, 0.7f);
        }
        break;
    }
    if (em_alt_mode_ck(self) == 1) {
        if ((self->field_0x761 & 1) == 0) {
            self->field_0x761 |= 1;
        }
    } else if ((self->field_0x761 & 1) != 0) {
        self->field_0x761 &= 0xFE;
    }
}

/* 0x801CA004 (0x16C): fades the glow toward the alternate mode and drives the three MHchar K-colour alphas. */
void em004_tev_color_update(struct _ENEMY_WORK* self) {
    _GXColor color;
    u32 armed;
    if (em_alt_mode_ck(self) == 1) {
        self->em004_0x328.fade_0x328 = 1.275f + self->em004_0x328.fade_0x328;
        if (self->em004_0x328.fade_0x328 > 255.0f) {
            self->em004_0x328.fade_0x328 = 255.0f;
        }
    } else {
        self->em004_0x328.fade_0x328 -= 1.275f;
        if (self->em004_0x328.fade_0x328 < 0.0f) {
            self->em004_0x328.fade_0x328 = 0.0f;
        }
    }
    ((MHchar*)self->char_0x024)->getTevKColor(3, GX_KCOLOR1, &color);
    color.a = self->em004_0x328.fade_0x328;

    ((MHchar*)self->char_0x024)->setTevKColor(3, GX_KCOLOR1, &color);
    ((MHchar*)self->char_0x024)->getTevKColor(1, GX_KCOLOR1, &color);
    color.a = self->em004_0x328.fade_0x328;

    ((MHchar*)self->char_0x024)->setTevKColor(1, GX_KCOLOR1, &color);
    armed = 0;
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 4 && ((s32)self->area_no == 4 || (s32)self->area_no == 6)) {
        armed = 1;
    }
    ((MHchar*)self->char_0x024)->getTevKColor(2, GX_KCOLOR3, &color);
    if (armed == 1) {
        color.a = 0xFF;
    } else {
        color.a = 0;
    }
    ((MHchar*)self->char_0x024)->setTevKColor(2, GX_KCOLOR3, &color);
}

/* 0x801CA170 (0xE8): on map kind 4, finds the area's lookup entry and its record keyed `key`: returns the
 * record's point count and stores its point list through `out`. */
u32 em004_route_points_get(struct _ENEMY_WORK* self, u8 key, u8** out) {
    u8 i;
    if (stage_map_kind_get(self->field_0x1E0) != 4) {
        return 0;
    }
    for (i = 0; em004_route_tbl[i].key != 0xFF; i++) {
        if ((u8)self->area_no == em004_route_tbl[i].key) {
            u8 j;
            const EmLookupSub* sub;
            sub = em004_route_tbl[i].subs;
            for (j = 0; j < em004_route_tbl[i].count; j++, sub++) {
                if (sub->key == key) {
                    *out = sub->points;
                    return sub->count;
                }
            }
            break;
        }
    }
    return 0;
}

/* 0x801CA258 (0x1B4): picks the route point nearest the target among the points of the first armed motion
 * slot's kind; returns its index offset by the kind, or 0xFF. */
u8 em004_route_point_pick(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    u8* points;
    u8 kind;
    u8 i;
    u8 count;
    u8 j;
    u8 best;
    f32 best_dist;
    VEC3_ctor(&pos);
    if ((u8)self->field_0x1E2 != 1 || (u8)self->field_0x1E3 != 1) {
        return 0xFF;
    }
    kind = 0;
    for (i = 0; i < 10; i++) {
        EmMotionSlot* slot = &self->slots_0x244[i];
        if (slot->flags == 0) {
            break;
        }
        if ((slot->flags & 0x800) != 0 && slot->kind != 0) {
            kind = slot->kind;
            break;
        }
    }
    if ((s32)kind == 0) {
        return 0xFF;
    }
    count = em004_route_points_get(self, kind, &points);
    if ((s32)count == 0) {
        return 0xFF;
    }
    best = 0xFF;
    for (j = 0; j < count; j++) {
        f32 dist;
        em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | points[j]), &pos);
        dist = calcDistanceSqXZ((VEC3*)&self->vec_0x36C, (VEC3*)&pos);
        if ((s32)j == 0 || dist < best_dist) {
            best_dist = dist;
            best = j;
        }
    }
    if (best != 0xFF) {
        best += (u8)((kind - 1) * 10);
    }
    return best;
}

/* 0x801CA40C (0xC0): the program's state query: per `kind`, the ready byte, the three +0x330 timers or the
 * nearest route point. */
u8 em004_state_query(struct _ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        if ((s32)self->field_0x23C != 0) {
            return 1;
        }
        break;
    case 1:
        if (self->em004_0x328.timer_0x330 <= 0) {
            return 1;
        }
        break;
    case 2: {
        u8 point = em004_route_point_pick(self);
        if (point == 0xFF) {
            return 0xFF;
        }
        return point;
    }
    case 3:
        if (self->em004_0x328.count_0x332 >= 300) {
            return 1;
        }
        break;
    case 4:
        if (self->em004_0x328.timer_0x334 <= 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x801CA4CC (0x108): picks the motion by the map id, places the enemy on its route point and raises it. */
void em004_death_action_get(struct _ENEMY_WORK* self, u8* out_mode, u8* out_flag) {
    em_fall_height_get(self);
    em_fall_start(self);
    *out_mode = 0x0C;
    *out_flag = 0;
    switch (stage_map_kind_get(self->field_0x1E0)) {
    case 4:
        if ((s32)(u8)self->area_no == 4)
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        break;
    case 8:
        if ((s32)(u8)self->area_no == 1)
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        break;
    case 9:
        if ((s32)(u8)self->area_no == 0)
            em_data_point_get(self, (u16)((self->area_no & 0xF) << 8), &self->pos);
        break;
    }
    self->pos.y += 5000.0f;
}

/* 0x801CA5D4 (0x4): an empty body. */
void em004_noop(struct _ENEMY_WORK* self) {
}

/* 0x801CA5D8 (0x98): the part check: in the idle state, kind 0 tests the effect, kinds 3/5 the part's damage
 * level. */
s32 em004_part_ck(struct _ENEMY_WORK* self, u8 kind) {
    if ((s32)self->field_0x1E2 == 0) {
        switch (kind) {
        case 0:
            if (em_eft_active_ck(self) == 1) {
                return 1;
            }
            break;
        case 3:
            if (em_parts_damage_level_get(self, 3) >= 2) {
                return 1;
            }
            break;
        case 5:
            if (em_parts_damage_level_get(self, 5) >= 2) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 0x801CA670 (0x58): on a part-2 break, drops an item at the body. */
void em004_part_break(struct _ENEMY_WORK* self, u8 part) {
    if ((s32)part == 2 && em_parts_damage_level_get(self, 2) == 2) {
        em_drop_item_spawn(self, &self->pos, 100);
    }
}

/* 0x801CA6C8 (0x128): the area-change check on map kind 4: the area break, follow, roam or escape tests. */
s32 em004_area_change_ck(struct _ENEMY_WORK* self, u16 timer) {
    u8 map = stage_map_kind_get(self->field_0x1E0);
    s32 moved;
    u32 area;
    if ((s32)map != 4) {
        return 0;
    }
    moved = 0;
    area = 0xFF;
    if ((s32)map == 4) {
        area = 4;
    }
    if (area != 0xFF) {
        switch (em_area_break_ck_v(self, area)) {
        case 1:
            moved = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (moved == 0) {
        if (em_pack_area_pick_v(self, 0, (s32)map == 4 ? (u8)5 : (u8)0xFF, timer, 0, (u8*)em004_follow_area_tbl) == 1) {
            return 1;
        }
    }
    if (em_area_roam_pick(self, timer) == 1) {
        return 1;
    }
    return em_area_leave_ck_v(self) == 1;
}

/* 0x801CA7F0 (0x80): fills a spawn record: kind 0x1C, the fixed vector and the three scalar fields. */
void em004_spawn_rec_fill(EmSpawnRec* rec, u8 a, u16 b, u16 c) {
    VEC3 v;
    setVec3(&v, 0.0f, 0.0f, 50.0f);
    rec->id = 0x1C;
    copyVec3(&rec->pos, &v);
    rec->field_0x10 = a;
    rec->field_0x12 = b;
    rec->field_0x14 = c;
}

/* 0x801CA870 (0x6C): seeds the two offset vectors. */
void em004_offset_init(void) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    assignVec3(&em004_offset_vec[0], (Vec*)setVec3(&a, 0.0f, 70.0f, 0.0f));
    assignVec3(&em004_offset_vec[1], (Vec*)setVec3(&b, 0.0f, -0.6f, 0.0f));
}

} /* extern "C" */
