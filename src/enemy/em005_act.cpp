/*
 * enemy/em005_act.cpp - enemy 005's program: the program table `em005_prog_tbl` and its entries (work clear, init,
 *   motion next, event notify, frame update, action run, motion events, glow update, state query, death action, part
 *   checks, area-change check), the step functions of actions 0-4, 7 and 10-13 with their `state_sub` dispatchers,
 *   the two sound-table players, the intro demo, the footstep effect dispatcher, the route-point and capsule tests,
 *   the user-data helper's constructor/destructor and MWCC's static initializer, with the unit's turn/flight tables,
 *   key curves, follow-area records, sound tables, demo key tracks and capsule table.
 * RANGE. .text 0x801CA8DC-0x801D71C4 (116 functions); extab 0x8000FE4C-0x80010164, extabindex
 *   0x8002BC5C-0x8002C100, .ctors 0x8056F358-0x8056F35C (`__sinit_\em005_act_cpp`), .rodata 0x80570410-0x80570500,
 *   .data 0x805B4F90-0x805B69D8 (`em005_prog_tbl` first), .bss 0x806A7B30-0x806A7B90, .sdata 0x80791B20-0x80791B38,
 *   .sdata2 0x80799220-0x807994F8.  Left edge: the function after `enemy/em004_act.cpp`'s static initializer;
 *   right edge: `__sinit_\em005_act_cpp` is this TU's static initializer and the 0.0 pool entry repeats at
 *   0x807994F8 from 0x801D71C4 on.  The functions are in address order; the `.data` order is the source order (the
 *   tables before the first function, the sound tables before their players, the demo tracks before the demo, the
 *   jump tables with their functions, the capsule table last).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi`
 *   pairs), `#pragma fp_contract off` (retail keeps `fmuls` + `fadds`) and `#pragma pool_data off` (each table is
 *   addressed with its own `lis`/`addi`).
 * NAMES. `em005_prog_tbl` is the runtime dump's name and opens the TU's `.data`; the `_act` suffix is a GUESS.  Every
 *   other function, table, record and field name defined here is a GUESS from its body, its `em005_prog_tbl` slot,
 *   its dispatcher case or the data that points at it: `em005_act<N>_*` steps are named by the action id
 *   `em005_action_run` dispatches on, the sound tables by the first motion that plays them.
 * RESIDUALS. Partial rows:
 *  - `em005_route_point_pick`: the count/target/index registers rotate (r29/r27/r28 in retail); 119 declaration
 *    orders and four loop shapes measured, none reaches retail's colouring;
 *  - `__sinit_\em005_act_cpp`: retail builds each capsule end point through an out-of-line constructor/copy
 *    constructor pair (`setVec3` returning `this`, then `assignVec3` from that result: `mr r4,r3`) and places those
 *    temporaries below the vector pairs'; the inline pair written here copies from the temporary's address
 *    (`addi r4,r1,N`) and orders the slots the other way.
 *   Data: the user-data helper's class table `em005_user_data_class_tbl` (0x805B69A8, 0x30 B, at the end of `.data`)
 *   is not emitted (it needs the `ResUserDataAc` class hierarchy, rule 10), so `.data` is 0x34 B short and the unit
 *   cannot flip; the helper's constructor stores the table address by hand.
 * SHAPES. A `u8` compared with `==` gives `cmplwi`; retail's `cmpwi` comes from a `(s32)` cast or a one-case `switch`,
 *   and a masked byte compared unsigned is `(u8)(x & m) == n`.  The aim angles are `u32` locals holding
 *   `(u16)(calcVecAng2(...) - field_0x1C0)`; the random side offset is written `angle + 0x4000 + bits % 0x2000`.
 *   A scale multiplied in is the call itself on the right (`E * get_em_chg_scale(self)`: MWCC calls it first and
 *   keeps `E` first in the `fmuls`); a negated field is `*= -1.0f`.  The sound-table player lists its cases in
 *   retail's code order; several `switch`es put `default:` first; the rotation record handed to
 *   `em_move_offset_rot_apply` is one function-scope `EmRotWord3`.  Callees whose call sites were compiled against
 *   other spellings than the owner header's are called through `_v` cast views (the float listed before the pointer
 *   for `em_ground_impact_set`/`capsule_box_from_points`, a float argument for `em_ground_ck`).
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
#include "camera/camera_main.h"
#include "enemy/em_common.h"
#include "enemy/em_model.h"
#include "enemy/em_ground_impact_set.h"
#include "stage/stg_ground_impact_set.h"
#include "ef/vec3_div.h"
#include "enemy/em_any_player_seen_ck.h"
#include "enemy/em_patrol_step.h"
#include "lobby/fn_801E0ADC.h"
#include "ef/eft_capsule_ctor.h"
#include "ef/eft_shape_init.h"
#include "Pl/pl_hit_sphere.h"
#include "Pl/fn_8028F66C.h"
#include "Pl/plw.h"
#include "enemy/fn_80137604.h"

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* Callees whose owner prototypes differ from the spelling retail's call sites were compiled with: a cast call is the
 * same direct call. */
#define em_ground_ck_f ((u32 (*)(struct _ENEMY_WORK*, f32))em_ground_ck)
#define em_ground_impact_set_v ((void (*)(struct _ENEMY_WORK*, u8, u8, f32, nw4r::math::VEC3*))em_ground_impact_set)
#define em_area_break_ck_v ((u8 (*)(struct _ENEMY_WORK*, u8))em_area_break_ck)
#define em_pack_area_pick_v ((u32 (*)(struct _ENEMY_WORK*, u8, u8, u16, u8*, u8*))em_pack_area_pick)
#define em_area_leave_ck_v ((s32 (*)(struct _ENEMY_WORK*))em_area_leave_ck)
#define capsule_box_from_points_v ((void (*)(void*, void*, f32, void*))capsule_box_from_points)
#define em_res_user_data_dtor_v ((void (*)(void*, s16))em_res_user_data_dtor)
#define em_move_offset_step_update_v ((u32 (*)(struct _ENEMY_WORK*, void*))em_move_offset_step_update)

/* The `ShellSetFuncs` slot spelled without its trailing table argument: the shared slot type hoists the table
 * load, retail loads it last. */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

#define assignVec3_c1 ((void (*)(VEC3*, VEC3*))assignVec3)

/* A point and its per-frame step, built by MWCC's `__sinit`: each vector is set through `setVec3` into a temporary
 * and copied with `assignVec3`.  size: 0x18 */
struct Em005VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (0, y, 0) and (0, dy, 0). */
    Em005VecPair(f32 y, f32 dy)
    {
        VEC3 b;
        VEC3 a;

        assignVec3_c1(&pos, setVec3(&a, 0.0f, y, 0.0f));
        assignVec3_c1(&step, setVec3(&b, 0.0f, dy, 0.0f));
    }
};

/* One capsule end point: built through `setVec3` and copied into place with `assignVec3`.  size: 0x0C */
struct Em005CapsulePoint {
    /* +0x00 */ VEC3 pos;

    Em005CapsulePoint(f32 x, f32 y, f32 z)
    {
        setVec3(&pos, x, y, z);
    }
    Em005CapsulePoint(const Em005CapsulePoint& other)
    {
        assignVec3_c1(&pos, (VEC3*)&other.pos);
    }
};

/* One capsule of the three-capsule table: its two end points and the radius.  size: 0x1C */
struct Em005Capsule {
    /* +0x00 */ Em005CapsulePoint start;
    /* +0x0C */ Em005CapsulePoint end;
    /* +0x18 */ f32 radius;
};

/* The user-data helper `em005_init` allocates (0xC B, `operator new(0xC)`): the class table its constructor stores,
 * then the base record's work pointer and flag word (`enemy/fn_80138074.h`'s `EmUserData`).  size: 0x0C */
struct Em005UserData {
    /* +0x00 */ u32* class_tbl_0x00;
    /* +0x04 */ struct _ENEMY_WORK* work_0x04;
    /* +0x08 */ u32 flags_0x08;
};

/* The three-word rotation record `em_move_offset_rot_apply` takes.  size: 0x0C */
struct EmRotWord3 {
    /* +0x0 */ u32 x;
    /* +0x4 */ u32 y;
    /* +0x8 */ u32 z;
};

/* Defined at the end of the unit (MWCC's `__sinit` builds them); the sound records and the capsule test read them first. */
extern Em005VecPair em005_vec_pair_0;
extern Em005VecPair em005_vec_pair_1;
extern Em005VecPair em005_vec_pair_2;
extern Em005VecPair em005_vec_pair_3;
extern Em005Capsule em005_capsule_tbl[3];
/* The user-data helper class table (`.data`, not emitted: see RESIDUALS). */
extern u32 em005_user_data_class_tbl[];


extern "C" {
s32 em005_slot_angle_avg(struct _ENEMY_WORK* self, u32 kind);
u32 em005_slot_turn_step(struct _ENEMY_WORK* self, u16 step, u8 kind);
void em005_work_clear(_ENEMY_WORK* self);
void em005_init(struct _ENEMY_WORK* self, u8 mode);
Em005UserData* em005_user_data_ctor(Em005UserData* self);
void em005_motion_next(struct _ENEMY_WORK* self, u8 *arg1, u8 *arg2);
void em005_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub);
void em005_frame_update(struct _ENEMY_WORK* self);
void em005_fall_arm_act3_11(_ENEMY_WORK* self);
void em005_fall_arm_act3_20(_ENEMY_WORK* self);
void em005_arm_act7_5(_ENEMY_WORK* self);
void em005_arm_act7_6(_ENEMY_WORK* self);
void em005_act0_motion1(_ENEMY_WORK* self);
void em005_act0_motion2(_ENEMY_WORK* self);
void em005_act0_motion2_alt(_ENEMY_WORK* self);
void em005_act0_fall(_ENEMY_WORK* self);
void em005_act0_walk_end(_ENEMY_WORK* self);
void em005_act0_run(_ENEMY_WORK* self);
void em005_act1_slam(struct _ENEMY_WORK* self, u8 arg1);
void em005_act1_motion12(_ENEMY_WORK* self);
void em005_act1_motion13(_ENEMY_WORK* self);
void em005_act1_motion14(_ENEMY_WORK* self);
void em005_act1_rest(_ENEMY_WORK* self);
void em005_act1_motion7C(_ENEMY_WORK* self);
void em005_act1_motion4(_ENEMY_WORK* self, u8 a);
void em005_act1_motion3(_ENEMY_WORK* self);
void em005_act1_motion2(_ENEMY_WORK* self, u8 a);
void em005_act1_run(struct _ENEMY_WORK* self);
void em005_act2_walk(_ENEMY_WORK* self, u8 a);
void em005_act2_run_approach(_ENEMY_WORK* self, u8 a);
void em005_act2_turn(_ENEMY_WORK* self);
void em005_act2_turn_wide(_ENEMY_WORK* self, u8 a);
void em005_act2_charge(struct _ENEMY_WORK* self, u8 side, u8 range, u8 fast);
void em005_act2_side_step(_ENEMY_WORK* self, u8 a);
void em005_act2_back_hop(struct _ENEMY_WORK* self, u8 mode, u8 sub, u8 extra);
void em005_act2_back_hop_busy(struct _ENEMY_WORK* self, u32 a, u32 b);
void em005_act2_motionD1(struct _ENEMY_WORK* self, u32 mode);
void em005_act2_run(struct _ENEMY_WORK* self);
void em005_act3_takeoff(struct _ENEMY_WORK* self);
void em005_act3_takeoff_busy(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_land(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_noop(struct _ENEMY_WORK* self);
void em005_act3_hover_aim(struct _ENEMY_WORK* self);
void em005_act3_fly_area(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_climb(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_hover_turn(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_dive_attack(struct _ENEMY_WORK* self);
void em005_act3_hover_turn_back(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_hover_motion1F(struct _ENEMY_WORK* self);
void em005_act3_hover_motion20(struct _ENEMY_WORK* self);
void em005_act3_hover_approach(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_water_motion1B(struct _ENEMY_WORK* self);
void em005_act3_motion2A(struct _ENEMY_WORK* self);
void em005_act3_dive_land(struct _ENEMY_WORK* self);
void em005_act3_water_rise(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_water_turn(struct _ENEMY_WORK* self);
void em005_act3_water_approach(struct _ENEMY_WORK* self);
void em005_act3_water_height_seek(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_height_seek(struct _ENEMY_WORK* self, u8 mode);
void em005_act3_run(struct _ENEMY_WORK* self);
void em005_act4_motion2A(struct _ENEMY_WORK* self);
void em005_act4_motion2E(struct _ENEMY_WORK* self);
void em005_act4_motion1E(struct _ENEMY_WORK* self);
void em005_act4_land(struct _ENEMY_WORK* self);
void em005_act4_turn(struct _ENEMY_WORK* self);
void em005_act4_approach(struct _ENEMY_WORK* self);
void em005_act4_leap(struct _ENEMY_WORK* self, u8 mode);
void em005_act4_charge(struct _ENEMY_WORK* self, u8 mode);
void em005_act4_run(struct _ENEMY_WORK* self);
void em005_act7_swing(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_fireball(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_turn(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_motionDE(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_jump(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void em005_act7_bite(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_lunge(struct _ENEMY_WORK* self);
void em005_act7_flip(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_swing2(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_flip_kick(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_rush(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void em005_act7_flip_aim(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void em005_act7_fire_spread(struct _ENEMY_WORK* self, u8 mode);
void em005_act7_charge(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void em005_act7_run(struct _ENEMY_WORK* self);
void em005_act10_se_play(struct _ENEMY_WORK* self);
void em005_act11_se_play(struct _ENEMY_WORK* self);
void em005_act12_land(struct _ENEMY_WORK* self);
void em005_act12_run(struct _ENEMY_WORK* self);
void em005_act13_fly_in(struct _ENEMY_WORK* self);
void em005_act13_dive(struct _ENEMY_WORK* self);
void em005_act13_lunge(struct _ENEMY_WORK* self);
void em005_act13_side_step(struct _ENEMY_WORK* self);
void em005_act13_roar(struct _ENEMY_WORK* self);
void em005_act13_flip(struct _ENEMY_WORK* self, u32 mode);
void em005_act13_demo(struct _ENEMY_WORK* self);
void em005_act13_demo_end(struct _ENEMY_WORK* self);
void em005_act13_run(struct _ENEMY_WORK* self);
void em005_action_run(struct _ENEMY_WORK* self);
void em005_body_spark(struct _ENEMY_WORK* self);
void em005_step_eft_set(struct _ENEMY_WORK* self, u8 kind, u8 id, u32 joint, s32 delta, f32 scale);
void em005_motion_events(struct _ENEMY_WORK* self);
void em005_tev_color_update(struct _ENEMY_WORK* self);
u32 em005_ally_near_ck(struct _ENEMY_WORK* self, u8 arg1);
u32 em005_act13_early_ck(struct _ENEMY_WORK* self);
u32 em005_capsule_point_ck(u8 index, nw4r::math::VEC3* point);
u32 em005_holder_in_area_ck(struct _ENEMY_WORK* self);
u8 em005_route_point_pick(struct _ENEMY_WORK* self, u32 first, u8 count, nw4r::math::VEC3* target);
u32 em005_state_query(struct _ENEMY_WORK* self, u32 arg1);
void em005_death_action_get(struct _ENEMY_WORK* self, u8* out1, u8* out2);
s32 em005_part_hit_ck(struct _ENEMY_WORK* self, u8 arg1);
s32 em005_part_ck(struct _ENEMY_WORK* self, u8 arg1);
void em005_part_arm(struct _ENEMY_WORK* self);
s32 em005_area_change_ck(struct _ENEMY_WORK* self, u16 timer);
void em005_spawn_rec_fill(EmSpawnRec* rec, u8 a1, u16 a2, u16 a3);
Em005UserData* em005_user_data_dtor(Em005UserData* self, s16 flag);
}

/* The em005 program table (`em005_prog_tbl`): the entry points the shared enemy program driver calls.  Slot
 * names are a GUESS from the functions they hold (the slots of `enemy/em004_act.cpp`'s table).
 * size: 0x70 */
struct Em005ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(struct _ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(struct _ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(struct _ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(struct _ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(struct _ENEMY_WORK* self, u8* state, u8* sub);
    /* +0x14 */ void (*event_notify_0x14)(struct _ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(struct _ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(struct _ENEMY_WORK* self);
    /* +0x24 */ u32 (*state_query_0x24)(struct _ENEMY_WORK* self, u32 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(struct _ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*part_arm_0x34)(struct _ENEMY_WORK* self);
    /* +0x38 */ s32 (*part_hit_ck_0x38)(struct _ENEMY_WORK* self, u8 part);
    /* +0x3C */ u32 pad_0x3C[2];
    /* +0x44 */ s32 (*part_ck_0x44)(struct _ENEMY_WORK* self, u8 part);
    /* +0x48 */ u32 pad_0x48[3];
    /* +0x54 */ s32 (*area_change_ck_0x54)(struct _ENEMY_WORK* self, u16 timer);
    /* +0x58 */ u32 pad_0x58[6];
};

Em005ProgTbl em005_prog_tbl = {
    em005_work_clear,
    em005_init,
    em005_frame_update,
    em005_action_run,
    em005_motion_next,
    em005_event_notify,
    em005_motion_events,
    0,
    em005_tev_color_update,
    em005_state_query,
    {0, 0},
    em005_death_action_get,
    em005_part_arm,
    em005_part_hit_ck,
    {0, 0},
    em005_part_ck,
    {0, 0, 0},
    em005_area_change_ck,
    {0, 0, 0, 0, 0, 0},
};

/* The `em_turn_seq_start`/`em_turn_seq_step` descriptor blocks (`.rodata`, 0x40 B each: a four-word head, then three
 * four-word rows whose first word carries the motion id in its high half) and the `em_fly_start`/`em_fly_step` table. */
extern "C" const u32 em005_turn_tbl[16] = {
    0x00000000, 0x00000046, 0x0000006E, 0x40000E39,
    0x000E0000, 0x0000000A, 0x00000000, 0x00000000,
    0x000A0000, 0x00000004, 0x00000000, 0x00000000,
    0x000B0000, 0x00000004, 0x00000000, 0x00000000,
};
extern "C" const u32 em005_turn_tbl_wide[16] = {
    0x0000000A, 0x00000018, 0x00000000, 0x40001555,
    0x000E0000, 0x00000004, 0x00000000, 0x00000000,
    0x000C0000, 0x00000002, 0x00000000, 0x00000000,
    0x000D0000, 0x00000002, 0x00000000, 0x00000000,
};
extern "C" const u32 em005_turn_tbl_water[16] = {
    0x00000000, 0x0000003C, 0x00000000, 0x40000E39,
    0x001B0000, 0x00000006, 0x00000000, 0x00000000,
    0x00250000, 0x00000006, 0x00000000, 0x00000000,
    0x00260000, 0x00000006, 0x00000000, 0x00000000,
};
extern "C" const u32 em005_fly_tbl[12] = {
    0x001F0000, 0x00000006, 0x00000000, 0x00000000,
    0x001F0000, 0x00000006, 0x00000000, 0x00000001,
    0x00230024, 0x42F00000, 0x00000000, 0x00000000,
};
/* The `em_key_curve_eval` curves (`{frame, value}` pairs, closed by a -1 frame). */
f32 em005_climb_curve[20] = {
    0.0f, 14.0f, 10.0f, 21.6f, 20.0f, 18.0f, 30.0f, 1.2f,
    50.0f, -1.2f, 70.0f, 6.6f, 90.0f, 9.0f, 100.0f, -1.2f,
    120.0f, 1.2f, -1.0f, 9.6f,
};
f32 em005_rise_curve[14] = {
    0.0f, 14.0f, 10.0f, 21.6f, 20.0f, 18.0f, 30.0f, 1.2f,
    50.0f, -1.2f, 52.0f, -0.8f, -1.0f, 0.0f,
};
f32 em005_takeoff_curve[14] = {
    0.0f, 77.0f, 8.0f, 57.2f, 16.0f, 24.2f, 24.0f, -5.5f,
    32.0f, -14.3f, 48.0f, -13.2f, -1.0f, 0.0f,
};
f32 em005_leap_rise_curve[14] = {
    0.0f, 6.0f, 20.0f, 12.0f, 40.0f, 19.0f, 60.0f, -2.0f,
    80.0f, -5.0f, 120.0f, 0.0f, -1.0f, 0.0f,
};
f32 em005_leap_fwd_curve[20] = {
    0.0f, 0.0f, 30.0f, 13.0f, 40.0f, 28.0f, 50.0f, 32.0f,
    60.0f, 36.0f, 70.0f, 15.0f, 80.0f, 8.0f, 90.0f, 2.0f,
    120.0f, 0.0f, -1.0f, 0.0f,
};
f32 em005_fire_spread_curve[10] = {
    0.0f, 0.0f, 70.0f, -15.0f, 90.0f, -30.0f, 114.0f, -40.0f,
    -1.0f, -10.0f,
};
f32 em005_flip_drop_curve[10] = {
    0.0f, 0.0f, 70.0f, 0.0f, 98.0f, -20.0f, 100.0f, 0.0f,
    -1.0f, 0.0f,
};
f32 em005_flip_rise_curve[16] = {
    0.0f, 28.0f, 20.0f, 10.0f, 50.0f, 13.0f, 70.0f, 0.0f,
    80.0f, -40.0f, 98.0f, -60.0f, 100.0f, 0.0f, -1.0f, 0.0f,
};
f32 em005_flip_fwd_curve[16] = {
    0.0f, -12.0f, 20.0f, -7.0f, 50.0f, 1.0f, 70.0f, 20.0f,
    80.0f, 40.0f, 98.0f, 60.0f, 100.0f, 10.0f, -1.0f, 0.0f,
};
f32 em005_flip_kick_rise_curve[10] = {
    0.0f, -44.0f, 10.0f, 30.0f, 30.0f, 0.0f, 126.0f, 0.0f,
    -1.0f, 0.0f,
};
f32 em005_flip_kick_fwd_curve[10] = {
    0.0f, 80.0f, 10.0f, 55.0f, 30.0f, 0.0f, 126.0f, 0.0f,
    -1.0f, 0.0f,
};
f32 em005_flip_end_rise_curve[16] = {
    0.0f, -44.0f, 10.0f, -30.0f, 20.0f, 60.0f, 30.0f, 12.0f,
    40.0f, 8.0f, 50.0f, 5.0f, 62.0f, 0.0f, -1.0f, 0.0f,
};
f32 em005_flip_end_fwd_curve[12] = {
    0.0f, 80.0f, 10.0f, 20.0f, 20.0f, 30.0f, 40.0f, 0.0f,
    62.0f, 0.0f, -1.0f, 0.0f,
};
f32 em005_jump_curve[14] = {
    0.0f, 0.0f, 4.0f, 15.0f, 20.0f, 100.0f, 36.0f, 30.0f,
    50.0f, 3.0f, 66.0f, 0.0f, -1.0f, 0.0f,
};
/* The follow-area records the area-change check hands `em_pack_area_pick` (a frame count, a packed id pair and an
 * 0xFFFF terminator). */
u32 em005_follow_area_tbl[3] = {0x14, 0xFA0005, 0xFFFF0000};
u32 em005_follow_area_tbl_near[3] = {0x14, 0x640005, 0xFFFF0000};

/* Averages the angles `calcVecAngXY` derives from the record's motion slots (mode 1 keeps only the slots whose value
 * lies in 0x6000..0xA000); -1 when no slot qualifies. */
s32 em005_slot_angle_avg(struct _ENEMY_WORK* self, u32 kind) {
    u32 angles[10];

    if (self->field_0x218 == 0) {
        return -1;
    }
    {
        u32 count = 0;
        s32 i;

        for (i = 0; i < 10; i++) {
            if (self->slots_0x244[i].flags == 0) {
                break;
            }
            if ((u8)kind == 1) {
                if ((u32)(self->slots_0x244[i].value - 24576) > 16384) {
                    continue;
                }
            }
            if ((self->slots_0x244[i].flags & 0x800) == 0) {
                continue;
            }
            {
                u32 angle_x;
                u32 angle_y;

                calcVecAngXY(&self->slots_0x244[i].vec, &angle_x, &angle_y);
                angles[count] = angle_y;
                count++;
            }
        }
        {
            s8 n;
            s32 sum;

            switch (count) {
            case 0:
                return -1;
            case 1:
                sum = angles[0];
                break;
            default:
                sum = angles[0];
                for (n = 1; (u32)n < count; n++) {
                    s32 delta = angles[n] - sum;

                    if (delta > 32768) {
                        delta -= 65536;
                    } else if (delta < -32768) {
                        delta += 65536;
                    }
                    sum += delta / (n + 1);
                }
                break;
            }
            return sum & 0xFFFF;
        }
    }
}

/* Steps the second rotation angle toward the slots' mean angle by `step`; 0 when no slot qualifies. */
u32 em005_slot_turn_step(struct _ENEMY_WORK* self, u16 step, u8 kind) {
    s32 angle = em005_slot_angle_avg(self, kind);

    if (angle == -1) {
        return 0;
    }
    self->field_0x1C0 = em_angle_approach((u16)angle, (u16)self->field_0x1C0, step);
    return 1;
}

/* 0x801CAA8C - clears the action block: the float, the word and the armed byte. */
void em005_work_clear(_ENEMY_WORK* self) {
    self->timer_0x328.field_0x328 = 0.0f;
    self->timer_0x328.field_0x32C = 0;
    self->action_0x328.armed_0x328.field_0x330 = 0xFF;
}

void em005_init(struct _ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 pos;
    VEC3_ctor(&pos);
    switch (self->field_0x010) {
    default:
        self->action_0x328.armed_0x328.field_0x330 = 0xFF;
        break;
    case 1:
        self->action_0x328.armed_0x328.field_0x330 = 0;
        break;
    case 2:
        self->action_0x328.armed_0x328.field_0x330 = 1;
        break;
    }
    if (mode == 0) {
        self->pos.y += 5000.0f;
        em_fall_height_get(self);
        em_fall_start(self);
        em_act_arm_unless_down(self, 3, 2);
        em_state_refresh(self);
    }
    if (em_res_user_data_ck(self) == 0) {
        Em005UserData* helper = (Em005UserData*)operator new(0xC);
        if (helper != 0) {
            em005_user_data_ctor(helper);
        }
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&pos, 0.0f, -35.0f, 55.0f);
        eft_em_attach_set(self, 0x1D, &pos, 1.1f, 0x1C72);
    }
}

/* The 0xC-byte user-data helper's constructor: the base record first, then this class's table. */
Em005UserData* em005_user_data_ctor(Em005UserData* self) {
    em_res_user_data_ctor(self);
    self->class_tbl_0x00 = em005_user_data_class_tbl;
    return self;
}

void em005_motion_next(struct _ENEMY_WORK* self, u8 *arg1, u8 *arg2) {
    u8 temp_r0;
    u8 temp_r0_2;
    u8 temp_r0_3;
    temp_r0 = *arg1;
    switch ((s32) temp_r0) {
    case 2:
        temp_r0_2 = *arg2;
        switch ((s32) temp_r0_2) {
        case 6:
            if (em_parts_damage_level_get(self, 3) >= 1U) {
                *arg2 = 0xB;
                return;
            }
        default:
            break;
        case 7:
            if (em_parts_damage_level_get(self, 2) >= 1U) {
                *arg2 = 0xC;
                return;
            }
            break;
        case 14:
        case 20:
            if (em_parts_damage_level_get(self, 3) >= 1U) {
                *arg2 = 0x10;
                return;
            }
            break;
        case 15:
        case 21:
            if (em_parts_damage_level_get(self, 2) >= 1U) {
                *arg2 = 0x11;
                return;
            }
            break;
        }
        break;
    case 7:
        temp_r0_3 = *arg2;
        switch (temp_r0_3) {
        case 0:
            if ((s32) (self->flags_0x836 & 1) == 0) {
                *arg2 = 0x21;
                return;
            }
            break;
        case 1:
            if ((s32) (self->flags_0x836 & 1) == 0) {
                *arg2 = 0x22;
                return;
            }
            break;
        case 2:
        case 44:
        case 45:
            if (em_eft_active_ck(self) == 1U) {
                *arg2 = 0x1F;
                return;
            }
            break;
        case 15:
            if ((em_parts_damage_level_get(self, 2) >= 1U) && (em_parts_damage_level_get(self, 3) >= 1U)) {
                *arg2 = 0x1C;
                return;
            }
            break;
        case 17:
            if ((em_parts_damage_level_get(self, 2) >= 1U) && (em_parts_damage_level_get(self, 3) >= 1U)) {
                *arg2 = 0x1D;
                return;
            }
            break;
        case 18:
            if (em_eft_active_ck(self) == 1U) {
                *arg2 = 0x29;
                return;
            }
            break;
        case 20:
            if ((em_parts_damage_level_get(self, 2) >= 1U) && (em_parts_damage_level_get(self, 3) >= 1U)) {
                *arg2 = 0x1E;
                return;
            }
            break;
        case 21:
        case 46:
            if (em_eft_active_ck(self) == 1U) {
                *arg2 = 0x20;
                return;
            }
            break;
        case 39:
            if (em_eft_active_ck(self) == 1U) {
                *arg2 = 0x2A;
            }
            break;
        }
        break;
    }
}

void em005_event_notify(struct _ENEMY_WORK* self, u8 kind, u8 sub) {
    switch (kind) {
    case 1:
        switch (sub) {
        case 3:
            em_bait_react(self);
            break;
        case 11:
            em_act_flag1_set(self);
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
        case 0x19:
            if (em_parts_damage_level_get(self, 2) >= 1) {
                self->flags_0x836 |= 0x8000;
            }
            break;
        case 0x1A:
            if (em_parts_damage_level_get(self, 3) >= 1) {
                self->flags_0x836 |= 0x4000;
            }
            break;
        case 0xCA:
            em_part_hit_set(self, 0, 0);
            break;
        }
        break;
    }
}

void em005_frame_update(struct _ENEMY_WORK* self) {
}

/* 0x801CAF70 - enters the 0x0B motion. */
void em005_fall_arm_act3_11(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    em_act_arm_unless_act11(self, 3, 0x0B);
    em_finish_flag_set(self);
}

/* 0x801CAFBC - enters the 0x14 motion. */
void em005_fall_arm_act3_20(_ENEMY_WORK* self) {
    em_fall_height_get(self);
    em_fall_start(self);
    em_act_arm_unless_act11(self, 3, 0x14);
    em_finish_flag_set(self);
}

/* 0x801CB008 - arms motion 7/5. */
void em005_arm_act7_5(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    em_act_arm_unless_act11(self, 7, 5);
    em_finish_flag_set(self);
}

/* 0x801CB050 - arms motion 7/6. */
void em005_arm_act7_6(_ENEMY_WORK* self) {
    em_move_mode_set(self, 0);
    em_act_arm_unless_act11(self, 7, 6);
    em_finish_flag_set(self);
}

/* 0x801CB098 - state 0 arms motion 1/0x0A, state 1 closes on the motion end. */
void em005_act0_motion1(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB114 - state 0 arms motion 2/4, state 1 closes on the motion end. */
void em005_act0_motion2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB190 - the same body as 0x801CB114 (the band's second 2/4 step). */
void em005_act0_motion2_alt(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 2, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB20C - state 0 re-seats the motion and arms 0x1A/6, state 1 runs it out. */
void em005_act0_fall(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_fall(self);
        break;
    }
}

/* 0x801CB28C - state 0 arms motion 0x1B/6, state 1 runs it out. */
void em005_act0_walk_end(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish_walk(self);
        break;
    }
}

/* 0x801CB308 - the sub-state dispatcher over the six step functions above. */
void em005_act0_run(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em005_act0_motion1(self);
        break;
    case 1:
        em005_act0_motion2(self);
        break;
    case 2:
        em005_act0_motion2_alt(self);
        break;
    case 3:
        em005_act0_fall(self);
        break;
    case 6:
        em005_act0_walk_end(self);
        break;
    }
}

void em005_act1_slam(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 sp8;
    VEC3_ctor(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 4, 0);
        if (arg1 == 1) {
            em_mot_speed_set(self, 0.75f);
        }
        self->timer_0x020 = 0;
        return;
    case 1:
        if ((s32) arg1 == 0) {
            if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                em_hit_window_set(self, 0, 0xF, 5);
                draw_shape_arm((u32) self, 0x1D, 0xA);
            }
            if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
                setVector3(&sp8, 0.0f, 0.0f, 200.0f);
                eft_em_spawn(self, 0, 0x1B, &sp8, 1.0f);
            }
            if (em_frame_check(self, 3, 142.0f, 264.0f) == 1) {
                if ((s32) (self->timer_0x020 & 7) == 0) {
                    setVector3(&sp8, 0.0f, 0.0f, 200.0f);
                    eft_em_spawn(self, 1, 0x1B, &sp8, 1.0f);
                }
                self->timer_0x020++;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x801CB500 - state 0 arms motion 0x12/0x0A, state 1 runs it out. */
void em005_act1_motion12(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 0x0A, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB57C - state 0 arms 0x13/6 and the 0x12C-frame timer, state 1 counts it down. */
void em005_act1_motion13(_ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        self->timer_0x020 = 0x12C;
        break;
    case 1:
        if (--self->timer_0x020 <= 0)
            em_state_set(self, 1, 3);
        break;
    }
}

/* 0x801CB618 - state 0 arms 0x14/8, state 1 runs it out. */
void em005_act1_motion14(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x14, 8, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB694 - the four-step 0x17/0x18/0x19 action. */
void em005_act1_rest(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        break;
    case 2:
        em_gauge_tick(self, 0.6f, 1, 0x14);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x19, 4, 0);
            em_tint_hold_end(self);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB7B0 - state 0 arms 0x7C/2, state 1 runs it out. */
void em005_act1_motion7C(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x7C, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB82C - state 0 arms 4/4; a nonzero `a` first re-seats the motion. */
void em005_act1_motion4(_ENEMY_WORK* self, u8 a) {
    if (a == 1)
        em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB8C0 - state 0 arms 3/4, state 1 runs it out. */
void em005_act1_motion3(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CB93C - state 0 arms 2/4, state 1 waits the `a`-selected frame window. */
void em005_act1_motion2(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, ((s32)(u8)a == 1) ? 120.0f : 180.0f, 0.0f) == 1)
            em_action_finish(self);
        break;
    }
}

void em005_act1_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em005_act1_slam(self, 0);
        break;
    case 1:
        em005_act1_motion12(self);
        break;
    case 2:
        em005_act1_motion13(self);
        break;
    case 3:
        em005_act1_motion14(self);
        break;
    case 4:
        em005_act1_rest(self);
        break;
    case 5:
        em005_act1_motion7C(self);
        break;
    case 6:
        em005_act1_motion4(self, 0);
        break;
    case 7:
        em005_act1_motion3(self);
        break;
    case 8:
        em005_act1_motion2(self, 0);
        break;
    case 9:
        em005_act1_motion2(self, 1);
        break;
    case 10:
        em005_act1_motion4(self, 1);
        break;
    case 11:
        em005_act1_slam(self, 1);
        break;
    }
}

/* 0x801CBA4C - state 0 arms 6/4 and starts the `a`-selected fade, state 1 waits it out. */
void em005_act2_walk(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 6, 4, 0);
        if ((s32)(u8)a != 1)
            em_approach_start(self, -500.0f, 0);
        else
            em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBB0C - state 0 arms 7/6, sets the motion rate and starts the `a`-selected fade. */
void em005_act2_run_approach(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 6, 0);
        em_mot_speed_set(self, 1.2f);
        if ((s32)(u8)a != 1)
            em_approach_start(self, -500.0f, 0);
        else
            em_approach_start(self, 0.0f, 0);
        break;
    case 1:
        if (em_approach_step(self, 0, 0x40) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBBD8 - state 0 starts the `em005_turn_tbl` table effect, state 1 waits it out. */
void em005_act2_turn(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em005_turn_tbl, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em005_turn_tbl) == 1)
            em_action_finish(self);
        break;
    }
}

/* 0x801CBC64 - state 0 starts the `em005_turn_tbl_wide` table effect (and the 0x482 fade), state 1 waits. */
void em005_act2_turn_wide(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em005_turn_tbl_wide, 0, 1, 0);
        if (self->field_0x482 == 1)
            em_mot_speed_set(self, 1.5f);
        break;
    case 1:
        if (a == 1 && em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        } else if (em_turn_seq_step(self, (void*)em005_turn_tbl_wide) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* The side charge: turns until the target lies within 90 degrees, picks a point beside it (600 or 200 out, capped
 * at 3000 from the body), runs there over 22 frames, then turns back and finishes with the side's closing motion. */
void em005_act2_charge(struct _ENEMY_WORK* self, u8 side, u8 range, u8 fast) {
    nw4r::math::VEC3 point;
    nw4r::math::VEC3 offset;
    u32 angle;

    VEC3_ctor(&point);
    VEC3_ctor(&offset);
    switch (self->state) {
    case 0:
        em_move_mode_set(self, 0);
        em_stamina_add(self, -10);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000) {
            angle = 0x10000 - angle;
        }
        if (angle < 0x4000) {
            self->state = 2;
            switch (side) {
            case 0:
            case 2:
                em_mot_set(self, 0xC9, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x4000 + self->bits_0x1EC % 0x2000;
                break;
            case 1:
            case 3:
                em_mot_set(self, 0xCA, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) - 0x4000 - self->bits_0x1EC % 0x2000;
                break;
            }
            if (fast == 1) {
                em_mot_speed_set(self, 1.1f);
            }
            switch (range) {
            case 0:
                setVector3(&offset, 0.0f, 0.0f, 600.0f);
                break;
            case 1:
                setVector3(&offset, 0.0f, 0.0f, 200.0f);
                break;
            }
            rotVecY(&offset, angle);
            {
                nw4r::math::VEC3 sum;
                addVec3(&sum, &self->vec_0x36C, &offset);
                copyVec3(&point, &sum);
            }
            if (calcDistanceSqXZ(&point, &self->pos) > 9000000.0f) {
                setVector3(&offset, 0.0f, 0.0f, 3000.0f);
                rotVecY(&offset, calcVecAng2(&self->pos, &point));
                {
                    nw4r::math::VEC3 sum;
                    addVec3(&sum, &self->pos, &offset);
                    copyVec3(&point, &sum);
                }
            }
            em_move_vec_clr(self);
            {
                f32 scale = get_em_base_scale(self);
                nw4r::math::VEC3 move;
                nw4r::math::VEC3 step;
                nw4r::math::VEC3 diff;
                subVec3(&diff, &point, &self->pos);
                vec3_div(&step, &diff, 22.0f);
                vec3_scale(&move, &step, scale);
                copyVec3(&self->offset_0x30C.vec_0x310, &move);
            }
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->timer_0x020 = (s16)(calcVecAng2(&point, &self->vec_0x36C) - self->field_0x1C0);
            break;
        }
        self->state++;
        em_turn_seq_start(self, (void*)em005_turn_tbl_wide, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em005_turn_tbl_wide) == 1) {
            self->state++;
            switch (side) {
            case 0:
            case 2:
                em_mot_set(self, 0xC9, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x4000 + self->bits_0x1EC % 0x2000;
                break;
            case 1:
            case 3:
                em_mot_set(self, 0xCA, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) - 0x4000 - self->bits_0x1EC % 0x2000;
                break;
            }
            if (fast == 1) {
                em_mot_speed_set(self, 1.1f);
            }
            switch (range) {
            case 0:
                setVector3(&offset, 0.0f, 0.0f, 600.0f);
                break;
            case 1:
                setVector3(&offset, 0.0f, 0.0f, 200.0f);
                break;
            }
            rotVecY(&offset, angle);
            {
                nw4r::math::VEC3 sum;
                addVec3(&sum, &self->vec_0x36C, &offset);
                copyVec3(&point, &sum);
            }
            if (calcDistanceSqXZ(&point, &self->pos) > 9000000.0f) {
                setVector3(&offset, 0.0f, 0.0f, 3000.0f);
                rotVecY(&offset, calcVecAng2(&self->pos, &point));
                {
                    nw4r::math::VEC3 sum;
                    addVec3(&sum, &self->pos, &offset);
                    copyVec3(&point, &sum);
                }
            }
            em_move_vec_clr(self);
            {
                f32 scale = get_em_base_scale(self);
                nw4r::math::VEC3 move;
                nw4r::math::VEC3 step;
                nw4r::math::VEC3 diff;
                subVec3(&diff, &point, &self->pos);
                vec3_div(&step, &diff, 22.0f);
                vec3_scale(&move, &step, scale);
                copyVec3(&self->offset_0x30C.vec_0x310, &move);
            }
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->timer_0x020 = (s16)(calcVecAng2(&point, &self->vec_0x36C) - self->field_0x1C0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            switch (side) {
            case 0:
            case 2:
                em_mot_set(self, 0xF4, 0, 0);
                break;
            case 1:
            case 3:
                em_mot_set(self, 0xF5, 0, 0);
                break;
            }
            if (fast == 1) {
                em_mot_speed_set(self, 1.1f);
            }
            addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        }
        break;
    case 3:
        em_turn_in_window(self, 0.0f, 22.0f, self->timer_0x020);
        addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            switch (side) {
            case 0:
                em_mot_set(self, 0xF0, 0, 0);
                if (fast == 1) {
                    em_mot_speed_set(self, 1.1f);
                }
                em_hit_window_set_default(self, 0, 0x1A);
                break;
            case 1:
                em_mot_set(self, 0xF1, 0, 0);
                if (fast == 1) {
                    em_mot_speed_set(self, 1.1f);
                }
                em_hit_window_set_default(self, 0, 0x1B);
                break;
            case 2:
                em_mot_set(self, 0xF2, 0, 0);
                break;
            case 3:
                em_mot_set(self, 0xF3, 0, 0);
                break;
            }
            angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (angle >= 0x8000) {
                angle = 0x10000 - angle;
            }
            self->timer_0x020 = (s16)angle;
            if (self->timer_0x020 > 0x2000) {
                self->timer_0x020 = 0x2000;
            }
        }
        break;
    case 4:
        switch (side) {
        case 0:
        case 1:
            if (em_frame_check(self, 3, 0.0f, 26.0f) == 1) {
                f32 deg = (360.0f * (f32)self->timer_0x020) / 65536.0f;
                em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * ((deg / 26.0f) * get_em_base_scale(self))) / 360.0f));
            }
            break;
        case 2:
        case 3:
            if (em_frame_check(self, 0, 60.0f, 0.0f) == 1) {
                em_mot_speed_set(self, 0.8f);
            }
            if (em_frame_check(self, 3, 100.0f, 130.0f) == 1) {
                f32 deg = (360.0f * (f32)self->timer_0x020) / 65536.0f;
                em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * ((deg / 30.0f) * get_em_base_scale(self))) / 360.0f));
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CC5DC - the strafe/approach action: state 0 seats the offset vector, state 1 steers it. */
void em005_act2_side_step(_ENEMY_WORK* self, u8 a) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_move_vec2_clr(self);
        switch (a) {
        case 0: {
            em_mot_set(self, 0xD2, 4, 0);
            em_mot_speed_set(self, 1.2f);
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.x = -15.625f * rate * scale;
            break;
        }
        case 1: {
            em_mot_set(self, 0xD3, 4, 0);
            em_mot_speed_set(self, 1.2f);
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.x = 15.625f * rate * scale;
            break;
        }
        }
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        em_hit_window_set_default(self, 0, 0x18);
        break;
    case 1:
        switch (a) {
        case 0:
            em_turn_in_window(self, 0.0f, 64.0f, 0x4000);
            break;
        case 1:
            em_turn_in_window(self, 0.0f, 64.0f, -0x4000);
            break;
        }
        if (em_frame_check(self, 3, 0.0f, 64.0f) == 1)
            CancelFade(self);
        if (em_mot_end_ck(self) == 1)
            em_action_finish(self);
        break;
    }
}

void em005_act2_back_hop(struct _ENEMY_WORK* self, u8 mode, u8 sub, u8 extra) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xEC, 4, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0xED, 0, 0);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.z = -14.0f;
            self->offset_0x30C.vec_0x310.y = 10.0f;
            vec3_scale_in_place(&self->offset_0x30C.vec_0x310, get_em_base_scale(self));
            self->vec_0x31C.y = -0.5f * get_em_base_scale(self);
            em_move_offset_step(self, &self->field_0x1BC);
            return;
        }
        break;
    case 2:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.y < 0.0f) {
            self->offset_0x30C.vec_0x310.y = 0.0f;
        }
        if (em_ground_ck_f(self, em_fall_height_get(self) - 30.0f) == 1) {
            self->state = 4;
            em_mot_set(self, 0xEE, 0, 0);
            self->vec_0x31C.z = 0.5f;
            return;
        }
        if (em_mot_end_ck(self) == 1) {
            if (em_ground_ck_f(self, 100.0f + em_fall_height_get(self)) == 1) {
                self->state = 4;
                em_mot_set(self, 0xEE, 0, 0);
                self->vec_0x31C.z = 0.5f;
                return;
            }
            self->state = 3;
            em_mot_set(self, 0xE9, 6, 0);
            self->offset_0x30C.vec_0x310.y = -15.0f;
            self->vec_0x31C.z = 0.5f;
            return;
        }
        if (em_busy_ck(self) == 1) {
            if ((self->offset_0x30C.vec_0x310.y <= 0.0f) && (em005_slot_turn_step(self, 0xFFFF, 1) == 1U)) {
                em_reaction_req(self, 0, 0, 0);
                em_state_set(self, 7, 0x11);
                em_user_data_reenter(self);
                return;
            }
        }
        break;
    case 3:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > -20.0f) {
            self->offset_0x30C.vec_0x310.z = -20.0f;
        }
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_mot_set_blend(self, 0xEE, 4, 8, 1);
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->pos.y = self->field_0x20C + (em_fall_height_get(self) * get_em_scale(self));
            return;
        }
        if ((em_busy_ck(self) == 1) && (em005_slot_turn_step(self, 0xFFFF, 1) == 1U)) {
            em_reaction_req(self, 0, 0, 0);
            em_state_set(self, 7, 0x11);
            em_user_data_reenter(self);
            return;
        }
        break;
    case 4:
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > -20.0f) {
            self->offset_0x30C.vec_0x310.z = -20.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xEF, 0, 0);
            return;
        }
        break;
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act2_back_hop_busy(struct _ENEMY_WORK* self, u32 a, u32 b) {
    if ((u8) b == 1) {
        em_busy_set(self);
    }
    em005_act2_back_hop(self, (u8) a, 0, 0);
}

/* 0x801CCBC4 - one action's state machine: state 0 arms the motion and derives the aim angle, state 1
 * waits on `em_frame_check` and closes it. */
void em005_act2_motionD1(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD1, 4, 0);
        {
            u32 angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (angle >= 0x8000) {
                angle = 0x10000 - angle;
            }
            self->timer_0x020 = (s16)angle;
            if ((u32)self->timer_0x020 > 0x1555) {
                self->timer_0x020 = 0x1555;
            }
        }
        break;
    case 1:
        if ((u8)mode == 0 && em_frame_check(self, 3, 0.0f, 12.0f) == 1) {
            f32 v = get_em_base_scale(self);
            u16 angle = (u16)(s32)((65536.0f * (2.5f * v)) / 360.0f + 0.5f);
            em_turn_to_target(self, angle);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CCCE8 - the 22-way `state_sub` dispatcher: each arm tail-calls a step function; the default is the
 * implicit return (`cmplwi 0x15` + `bgtlr`). */
void em005_act2_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em005_act2_walk(self, 0); break;
    case 1: em005_act2_walk(self, 1); break;
    case 2: em005_act2_run_approach(self, 0); break;
    case 3: em005_act2_run_approach(self, 1); break;
    case 4: em005_act2_turn(self); break;
    case 5: em005_act2_turn_wide(self, 0); break;
    case 6: em005_act2_charge(self, 0, 0, 0); break;
    case 7: em005_act2_charge(self, 1, 0, 0); break;
    case 8: em005_act2_side_step(self, 0); break;
    case 9: em005_act2_side_step(self, 1); break;
    case 10: em005_act2_back_hop_busy(self, 0, 0); break;
    case 11: em005_act2_charge(self, 2, 0, 0); break;
    case 12: em005_act2_charge(self, 3, 0, 0); break;
    case 13: em005_act2_turn_wide(self, 1); break;
    case 14: em005_act2_charge(self, 0, 1, 0); break;
    case 15: em005_act2_charge(self, 1, 1, 0); break;
    case 16: em005_act2_charge(self, 2, 1, 0); break;
    case 17: em005_act2_charge(self, 3, 1, 0); break;
    case 18: em005_act2_motionD1(self, 0); break;
    case 19: em005_act2_motionD1(self, 1); break;
    case 20: em005_act2_charge(self, 0, 1, 1); break;
    case 21: em005_act2_charge(self, 1, 1, 1); break;
    }
}

/* 0x801CCE10 - the four-state open/close action: state 0 arms, state 1 waits and closes, state 2
 * aims at the reference position, state 3 releases the part pair. */
void em005_act3_takeoff(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em005_takeoff_curve);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x1A, 6, 0);
        }
        break;
    case 3:
        em_turn_to_target(self, 0x100);
        if (em_mot_end_ck(self) == 1) {
            em_act_set_unless_act11(self, 3, 2);
        }
        break;
    }
}

/* 0x801CCF50 - the same action shape as `em005_act3_takeoff` with the mode-1 prelude (`em_busy_set` +
 * `em_busy_timer_reset`) and a shorter close (state 2 ends through `em_action_finish_fall`). */
void em005_act3_takeoff_busy(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x29, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x43, 0, 0);
            em_move_vec_clr(self);
        }
        break;
    case 2:
        self->field_0x314 = em_key_curve_eval(self, em005_takeoff_curve);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CD068 - the five-state landing action: arm, wait under an `em_ground_ck` height test, then
 * descend onto the effect height (`field_0x20C`) at the effect scale, then release. */
void em005_act3_land(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (em_ground_ck_f(self, 1200.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x1D, 6, 0);
            self->field_0x314 = -(700.0f * get_em_scale(self) / 60.0f) * get_em_base_scale(self);
        }
        break;
    case 2:
        if (em_mot_frame_started_ck(self) == 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        {
            if (self->pos.y - self->field_0x20C < em_fall_height_get(self) * get_em_scale(self)) {
                self->pos.y = self->field_0x20C + em_fall_height_get(self) * get_em_scale(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x2E, 6, 0);
        }
        break;
    case 3:
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        {
            if (self->pos.y - self->field_0x20C < em_fall_height_get(self) * get_em_scale(self)) {
                self->pos.y = self->field_0x20C + em_fall_height_get(self) * get_em_scale(self);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CD2E8 - an empty body (the map's 4-byte `blr`). */
void em005_act3_noop(struct _ENEMY_WORK* self) {
}

/* 0x801CD2EC - the two-state "motion + ::UpdateValue" action: state 0 arms the motion, state 1
 * writes the aim-angle-derived value and hands the motion its angle pair. */
void em005_act3_hover_aim(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mat_blend_reset(self);
        em_mot_set(self, 0x1F, 0, 0);
        break;
    case 1: {
        f32 angle;
        u16 raw;
        u16 delta;
        em_target_pos_set(self, 0);
        raw = (u16)calcVecAng2(&self->pos, &self->vec_0x36C);
        delta = raw - self->field_0x1C0;
        if (delta != 0) {
            angle = 360.0f * (f32)(s16)delta / 65536.0f / 60.0f;
        } else {
            angle = 0.0f;
        }
        em_motion_blend_step(self, angle, 0.04f);
        em_mot_layer_blend_step(self, 0x23, 0x24, (u32)(s32)em_mot_frame_get(self), self->field_0x464);
        break;
    }
    }
}

/* 0x801CD400 - the target search and approach action: state 1 runs two sub-steps and picks its target
 * group through `em_ally_find`; early finishes `return` to the shared epilogue. */
void em005_act3_fly_area(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_ceiling_latch(self);
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 4) {
        if ((u32)(self->area_no - 4) <= 2) {
            em_shake_req_set(self);
        }
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fly_start(self, (void*)em005_fly_tbl);
        em_approach_start(self, 0.0f, 0x19);
        em_fall_height_get(self);
        em_move_rot_ground_step(self, &self->field_0x1BC);
        break;
    case 1:
        if (mode == 1) {
            switch (self->state_0x006) {
            case 0:
                if (self->field_0x1F9 == 0) {
                    struct _ENEMY_WORK* target;
                    self->state_0x006++;
                    if (em_eft_active_ck(self) == 1 || self->field_0x8A2 < 0xFA) {
                        target = em_ally_find(self, 0x1C, 0);
                    } else {
                        target = 0;
                    }
                    if (target != 0) {
                        self->state_0x007 = 1;
                        break;
                    }
                    if (self->field_0x43D == 1) {
                        em_any_player_seen_ck(self);
                    }
                }
                break;
            case 1:
                switch (self->state_0x007) {
                case 1: {
                    struct _ENEMY_WORK* target = em_ally_find(self, 0x1C, 1);
                    if (target != 0) {
                        em_reaction_req(self, 3, 2, target->group);
                        em_state_set(self, 0x0D, 0);
                        em_user_data_reenter(self);
                        return;
                    }
                    break;
                }
                case 2:
                    if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= 16000000.0f) {
                        em_state_set(self, 3, 8);
                        em_user_data_reenter(self);
                        return;
                    }
                    break;
                }
                break;
            }
        }
        if (em_approach_step(self, 0, 0) == 1) {
            switch (mode) {
            default:
                em005_fall_arm_act3_11(self);
                break;
            case 1:
                if (self->area_no == self->field_0x9F8 || self->field_0x9F8 == 0xFF) {
                    em005_fall_arm_act3_11(self);
                } else {
                    em_state_set(self, 3, 9);
                }
                break;
            case 2:
                if (self->field_0x1E7 == 0) {
                    em005_fall_arm_act3_11(self);
                } else {
                    em_patrol_step(self, 0);
                    em_act_set_unless_act11(self, 3, 0x0E);
                }
                break;
            }
        } else {
            switch (mode) {
            case 2:
                em_fly_step(self, 2, (void*)em005_fly_tbl);
                break;
            default:
                em_fly_step(self, 2, (void*)em005_fly_tbl);
                break;
            case 3:
                em_fly_step(self, 3, (void*)em005_fly_tbl);
                break;
            }
            em_fall_height_get(self);
            em_move_rot_ground_step(self, &self->field_0x1BC);
        }
        break;
    }
}

/* 0x801CD71C - aims at the reference height: run `em005_climb_curve` twice, then `em_lift_step`; the mode picks
 * the height (area height, own vector y, or `stg_area_ceiling_get`). */
void em005_act3_climb(struct _ENEMY_WORK* self, u8 mode) {
    f32 limit;
    em_busy_set(self);
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = em_key_curve_eval(self, em005_climb_curve);
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
        switch (mode) {
        default:
            limit = stg_area_ceiling_get(self->area_no) - 1000.0f;
            if (limit - self->field_0x20C < 2000.0f) {
                limit = 2000.0f + self->field_0x20C;
            }
            break;
        case 1:
            limit = 1500.0f + self->vec_0x36C.y;
            break;
        case 2:
            limit = self->vec_0x36C.y;
            break;
        }
        if (self->pos.y >= limit) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CD8A0 - the mode-1 prelude plus a two-state hold that turns the work record by 0x200 and
 * ends through `em_action_finish_fall`. */
void em005_act3_hover_turn(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_ceiling_latch(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CD944 - the six-state "fly up, hover and throw" action.  State 2 runs two sub-steps through
 * `em_move_offset_step_update` (whose return selects the state-4 hand-over) and state 3/4/5 close the motion. */
void em005_act3_dive_attack(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x3D, 6, 0);
            self->vec_0x36C.y += 500.0f * get_em_chg_scale(self);
            self->state_0x006 = 0;
        }
        break;
    case 2:
        em_hit_arm_set(self);
        switch (self->state_0x006) {
        case 0:
            em_turn_to_target_3d(self, 0x100, 0x100);
            if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
                self->state_0x006++;
                em_move_vec2_clr(self);
                self->field_0x318 = 5.0f;
                self->field_0x324 = 3.0f;
                em_approach_start(self, -300.0f, 0x10);
            }
            break;
        case 1:
            em_fall_height_get(self);
            em_move_offset_step_update_v(self, &self->field_0x1BC);
            em_approach_step(self, 0, 0x80);
            if (self->field_0x318 > 70.0f) {
                self->field_0x318 = 70.0f;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x3E, 6, 0);
        }
        break;
    case 3:
        em_hit_arm_set(self);
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            em_mot_set(self, 0x2D, 6, 0);
            self->state_0x006 = 0;
            self->field_0x314 = -10.0f;
            self->field_0x324 = -3.2f;
            em_fall_height_get(self);
            em_move_offset_step_update_v(self, &self->field_0x1BC);
        } else {
            em_fall_height_get(self);
            em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (self->field_0x318 > 70.0f) {
                self->field_0x318 = 70.0f;
            }
        }
        break;
    case 4: {
        u32 forward;
        switch (self->state_0x006) {
        case 0:
            em_fall_height_get(self);
            forward = em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
                self->state_0x006++;
                self->field_0x324 = -6.8f;
            }
            break;
        case 1:
            em_fall_height_get(self);
            forward = em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (self->field_0x318 < 0.0f) {
                self->field_0x318 = 0.0f;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            if (forward == 1) {
                self->state++;
                em_move_mode_set(self, 3);
                em_mot_set(self, 0x1D, 6, 0);
            } else if (em_busy_ck(self) == 1) {
                em_action_finish_fall(self);
            }
        }
        break;
    }
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_move_mode_set(self, 3);
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CDCDC - the mode-gated turn: state 0 arms, state 1 turns by 0x180 (mode 1) and ends. */
void em005_act3_hover_turn_back(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x2D, 6, 0);
        break;
    case 1:
        switch (mode) {
        case 1:
            em_turn_to_target(self, 0x180);
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CDD94 - the timer action: state 0 arms a 0x3C-frame wait, state 1 counts it down and hands
 * control back to `em005_fall_arm_act3_11` when it expires. */
void em005_act3_hover_motion1F(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1F, 4, 0);
        self->timer_0x020 = 0x3C;
        break;
    case 1: {
        s32 timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0) {
            em005_fall_arm_act3_11(self);
        }
        break;
    }
    }
}

/* 0x801CDE34 - the effect-scale action: state 0 arms with the two effect scales, state 1 clamps the
 * scale and ends through `em005_fall_arm_act3_11`. */
void em005_act3_hover_motion20(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_ceiling_latch(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x20, 6, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 15.0f;
        self->field_0x324 = 3.0f;
        break;
    case 1:
        em_fall_height_get(self);
        em_move_offset_step_update_v(self, &self->field_0x1BC);
        if (self->field_0x318 > 90.0f) {
            self->field_0x318 = 90.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            em005_fall_arm_act3_11(self);
        }
        break;
    }
}

/* 0x801CDF10 - the mode-1 turn/clamp action. */
void em005_act3_hover_approach(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
        em_ceiling_latch(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 10.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_fall(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 20.0f) {
                self->field_0x318 = 20.0f;
            }
        }
        break;
    }
}

/* 0x801CE014 - the mode-1 clamps plus a two-state open/close whose close hands over to
 * `em005_fall_arm_act3_20`. */
void em005_act3_water_motion1B(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em005_fall_arm_act3_20(self);
        }
        break;
    }
}

/* 0x801CE0C0 - the three-state aim-and-close action. */
void em005_act3_motion2A(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x44, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, 40.0f, 0.0f) == 1) {
            em005_fall_arm_act3_20(self);
        }
        break;
    }
}

/* 0x801CE190 - the four-state height-gated close: state 1 waits on `em_ground_ck` (which reads the
 * `em_fall_height_get` return in f1), state 2 re-arms the motion, state 3 closes it. */
void em005_act3_dive_land(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (em_ground_ck_f(self, em_fall_height_get(self)) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x2E, 6, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CE2B0 - the mode-1 clamp set plus a two-sub-step hold that closes when the work record
 * reaches its own vector's height. */
void em005_act3_water_rise(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1B, 6, 0);
        em_move_vec_clr(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = em_key_curve_eval(self, em005_rise_curve);
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
        f32 goal = self->vec_0x36C.y;
        if (self->pos.y >= goal) {
            em005_fall_arm_act3_20(self);
        }
        break;
    }
}

/* 0x801CE3F0 - the mode-1 clamps plus a two-state turn (state 1 turns by 0x200 and closes). */
void em005_act3_water_turn(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 0x200) == 1) {
            em005_fall_arm_act3_20(self);
        }
        break;
    }
}

/* 0x801CE4A0 - the mode-1 clamps plus an `em_approach_step`-gated two-state hold. */
void em005_act3_water_approach(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        em_approach_start(self, 0.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 10.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em005_fall_arm_act3_20(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 20.0f) {
                self->field_0x318 = 20.0f;
            }
        }
        break;
    }
}

/* 0x801CE5A0 - the effect-pair action: state 0 measures the height difference and seeds the two effect
 * scales, state 1 clamps `field_0x314` and runs `value_0x378` down. */
void em005_act3_water_height_seek(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1B, 6, 0);
        self->value_0x378 = self->pos.y - self->vec_0x36C.y;
        if (self->value_0x378 < 0.0f) {
            self->value_0x378 *= -1.0f;
        }
        em_move_vec2_clr(self);
        if (self->pos.y <= self->vec_0x36C.y) {
            self->field_0x314 = 10.0f;
            self->field_0x320 = 1.0f;
        } else {
            self->field_0x314 = -10.0f;
            self->field_0x320 = -1.0f;
        }
        break;
    case 1: {
        f32 limit;
        em_move_offset_step(self, &self->field_0x1BC);
        limit = (mode == 0) ? 20.0f : 60.0f;
        if (self->field_0x314 > limit) {
            self->field_0x314 = limit;
        } else if (self->field_0x314 < -limit) {
            self->field_0x314 = -limit;
        }
        self->value_0x378 = self->value_0x378 - vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= 0.0f) {
            em005_fall_arm_act3_20(self);
        }
        break;
    }
    }
}

/* 0x801CE71C - `em005_act3_water_height_seek`'s sibling with the other close (`em_action_finish_fall`) and mode 1's limit. */
void em005_act3_height_seek(struct _ENEMY_WORK* self, u8 mode) {
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    em_ceiling_latch(self);
    em_fx_flag2_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 0x1A, 6, 0);
        self->value_0x378 = self->pos.y - self->vec_0x36C.y;
        if (self->value_0x378 < 0.0f) {
            self->value_0x378 *= -1.0f;
        }
        em_move_vec2_clr(self);
        if (self->pos.y <= self->vec_0x36C.y) {
            self->field_0x314 = 10.0f;
            self->field_0x320 = 1.0f;
        } else {
            self->field_0x314 = -10.0f;
            self->field_0x320 = -1.0f;
        }
        break;
    case 1: {
        f32 limit;
        em_move_offset_step(self, &self->field_0x1BC);
        limit = (mode == 0) ? 20.0f : 60.0f;
        if (self->field_0x314 > limit) {
            self->field_0x314 = limit;
        } else if (self->field_0x314 < -limit) {
            self->field_0x314 = -limit;
        }
        self->value_0x378 = self->value_0x378 - vec3_len(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= 0.0f) {
            em_action_finish_fall(self);
        }
        break;
    }
    }
}

/* 0x801CE898 - the `state_sub` (+0x1E6) dispatcher that owns this whole band: 34 ways, each a tail
 * call into one of the range's step functions (case 19 is the empty arm). */
void em005_act3_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em005_act3_takeoff(self); break;
    case 1: em005_act3_takeoff_busy(self, 0); break;
    case 2: em005_act3_land(self, 0); break;
    case 3: em005_act3_noop(self); break;
    case 4: em005_act3_hover_aim(self); break;
    case 5: em005_act3_fly_area(self, 0); break;
    case 6: em005_act3_climb(self, 0); break;
    case 7: em005_act3_hover_turn(self, 0); break;
    case 8: em005_act3_dive_attack(self); break;
    case 9: em005_act3_fly_area(self, 1); break;
    case 10: em005_act3_hover_turn_back(self, 0); break;
    case 11: em005_act3_hover_motion1F(self); break;
    case 12: em005_act3_hover_motion20(self); break;
    case 13: em005_act3_climb(self, 1); break;
    case 14: em005_act3_fly_area(self, 2); break;
    case 15: em005_act3_hover_turn_back(self, 1); break;
    case 16: em005_act3_climb(self, 2); break;
    case 17: em005_act3_hover_approach(self, 0); break;
    case 18: em005_act3_fly_area(self, 3); break;
    case 19: break;
    case 20: em005_act3_water_motion1B(self); break;
    case 21: em005_act3_motion2A(self); break;
    case 22: em005_act3_dive_land(self); break;
    case 23: em005_act3_water_rise(self, 0); break;
    case 24: em005_act3_water_turn(self); break;
    case 25: em005_act3_water_approach(self); break;
    case 26: em005_act3_water_height_seek(self, 0); break;
    case 27: em005_act3_hover_turn(self, 1); break;
    case 28: em005_act3_takeoff_busy(self, 1); break;
    case 29: em005_act3_land(self, 1); break;
    case 30: em005_act3_hover_approach(self, 1); break;
    case 31: em005_act3_water_height_seek(self, 1); break;
    case 32: em005_act3_height_seek(self, 0); break;
    case 33: em005_act3_height_seek(self, 1); break;
    }
}

/* 0x801CE99C - the three-state aim/close action that ends through `em_action_finish_walk`. */
void em005_act4_motion2A(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2A, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            em_mot_set(self, 0x44, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, 40.0f, 0.0f) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CEA68 - the sibling of `em005_act4_motion2A` that closes through `em_action_finish`. */
void em005_act4_motion2E(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x2E, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x41, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CEB28 - the fade action: state 1 seeds the two effect scales from the model scale, state 2
 * switches between `em_move_offset_apply` and `CancelFade` on a frame window and closes. */
void em005_act4_motion1E(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1E, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, 4.0f, 0.0f) == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            em_move_vec2_clr(self);
            self->field_0x314 = 7.0f * get_em_base_scale(self);
            self->field_0x320 = -0.2f * get_em_base_scale(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, 76.0f, 0.0f) == 0) {
            em_move_offset_apply(self);
        } else {
            CancelFade(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* 0x801CEC44 - descend onto the effect height (`em005_act3_land`'s sibling): walk `pos.y` down to
 * `field_0x20C`, then `em_action_finish_walk`, or re-arm at 0x1B. */
void em005_act4_land(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x1A, 6, 0);
        em_dive_start(self);
        break;
    case 1:
        em_dive_step(self);
        if (em_ground_ck_f(self, 1200.0f) == 1) {
            self->state++;
            em_mot_set(self, 0x1D, 6, 0);
            self->field_0x314 = -(700.0f * get_em_scale(self) / 60.0f) * get_em_base_scale(self);
        }
        break;
    case 2:
        if (em_mot_frame_started_ck(self) == 0) {
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        {
            if (self->pos.y - self->field_0x20C < em_fall_height_get(self) * get_em_scale(self)) {
                self->pos.y = self->field_0x20C + em_fall_height_get(self) * get_em_scale(self);
                if (em_mot_end_ck(self) == 1) {
                    em_action_finish_walk(self);
                }
            } else if (em_mot_end_ck(self) == 1) {
                em_mot_set_ck(self, 0x1B, 6, 0);
            }
        }
        break;
    }
}

/* 0x801CEDE8 - the two-state file-row action: state 0 copies the `em005_turn_tbl_water` row, state 1 waits on
 * it through `em_turn_seq_step` and closes with `em_action_finish_walk`. */
void em005_act4_turn(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_turn_seq_start(self, (void*)em005_turn_tbl_water, 0, 0, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em005_turn_tbl_water) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CEE74 - the mode-1 clamp action with `em_approach_start` and the `em_approach_step` gate. */
void em005_act4_approach(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0x1B, 6, 0);
        em_approach_start(self, -700.0f, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 40.0f;
        self->field_0x324 = 1.0f;
        break;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish_walk(self);
        } else {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 70.0f) {
                self->field_0x318 = 70.0f;
            }
        }
        break;
    }
}

/* 0x801CEF44 - turns and re-seats: state 0 latches the rotation word, state 1 turns by +/-0x4000 (the mode's
 * sign), re-applies the word through `em_move_offset_rot_apply` and closes. */
void em005_act4_leap(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        if (mode == 0) {
            em_mot_set(self, 0xCB, 6, 0);
        } else {
            em_mot_set(self, 0xCC, 6, 0);
        }
        em_move_vec2_clr(self);
        self->field_0x37C = self->field_0x1C0;
        self->timer_0x328.field_0x328 =
            em_target_dist_ratio_get(self, 1.0f, 0.4f, 1450.0f);
        em_stamina_add(self, (u32)-0x0A);
        break;
    case 1: {
        struct EmRotWord3 rec;
        if (mode == 0) {
            em_turn_in_window(self, 60.0f, 120.0f, -0x4000);
            self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, em005_leap_rise_curve);
        } else {
            em_turn_in_window(self, 60.0f, 120.0f, 0x4000);
            self->offset_0x30C.vec_0x310.x = -em_key_curve_eval(self, em005_leap_rise_curve);
        }
        self->field_0x318 = self->timer_0x328.field_0x328 * em_key_curve_eval(self, em005_leap_fwd_curve);
        rec.x = 0;
        rec.y = self->field_0x37C;
        rec.z = 0;
        em_move_offset_rot_apply(self, &rec);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
    }
}

void em005_act4_charge(struct _ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 sp8C;
    nw4r::math::VEC3 sp80;
    nw4r::math::VEC3 sp74;
    nw4r::math::VEC3 sp68;
    nw4r::math::VEC3 sp5C;
    nw4r::math::VEC3 sp50;
    nw4r::math::VEC3 sp44;
    nw4r::math::VEC3 sp38;
    nw4r::math::VEC3 sp2C;
    nw4r::math::VEC3 sp20;
    nw4r::math::VEC3 sp14;
    nw4r::math::VEC3 sp8;
    f32 temp_f31;
    f32 temp_f31_2;
    u32 var_r3;
    u32 var_r30;
    u32 var_r30_2;
    VEC3_ctor(&sp8C);
    VEC3_ctor(&sp80);
    switch (self->state) {
    case 0:
        em_move_mode_set(self, 0);
        em_stamina_add(self, -10);
        var_r3 = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (var_r3 >= 0x8000) {
            var_r3 = 0x10000 - var_r3;
        }
        if (var_r3 < 0x4000U) {
            self->state = 2;
            if ((s32) mode == 0) {
                em_mot_set(self, 0xCD, 4, 0);
                var_r30 = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x4000 + self->bits_0x1EC % 0x2000;
            } else {
                em_mot_set(self, 0xCF, 4, 0);
                var_r30 = (calcVecAng2(&self->pos, &self->vec_0x36C) - 0x4000) - (self->bits_0x1EC % 0x2000);
            }
            setVector3(&sp80, 0.0f, 0.0f, 1100.0f);
            rotVecY(&sp80, var_r30);
            addVec3(&sp74, &self->vec_0x36C, &sp80);
            copyVec3(&sp8C, &sp74);
            if (calcDistanceSqXZ(&sp8C, &self->pos) > 9000000.0f) {
                setVector3(&sp80, 0.0f, 0.0f, 3000.0f);
                rotVecY(&sp80, calcVecAng2(&self->pos, &sp8C));
                addVec3(&sp68, &self->pos, &sp80);
                copyVec3(&sp8C, &sp68);
            }
            em_move_vec_clr(self);
            temp_f31 = get_em_base_scale(self);
            subVec3(&sp44, &sp8C, &self->pos);
            vec3_div(&sp50, &sp44, 30.0f);
            vec3_scale(&sp5C, &sp50, temp_f31);
            copyVec3(&self->offset_0x30C.vec_0x310, &sp5C);
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->timer_0x020 = (s32) (s16) (calcVecAng2(&sp8C, &self->vec_0x36C) - self->field_0x1C0);
            return;
        }
        self->state++;
        em_turn_seq_start(self, (void*)em005_turn_tbl_wide, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em005_turn_tbl_wide) == 1) {
            self->state++;
            if ((s32) mode == 0) {
                em_mot_set(self, 0xCD, 4, 0);
                var_r30_2 = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x4000 + self->bits_0x1EC % 0x2000;
            } else {
                em_mot_set(self, 0xCF, 4, 0);
                var_r30_2 = (calcVecAng2(&self->pos, &self->vec_0x36C) - 0x4000) - (self->bits_0x1EC % 0x2000);
            }
            setVector3(&sp80, 0.0f, 0.0f, 1100.0f);
            rotVecY(&sp80, var_r30_2);
            addVec3(&sp38, &self->vec_0x36C, &sp80);
            copyVec3(&sp8C, &sp38);
            if (calcDistanceSqXZ(&sp8C, &self->pos) > 9000000.0f) {
                setVector3(&sp80, 0.0f, 0.0f, 3000.0f);
                rotVecY(&sp80, calcVecAng2(&self->pos, &sp8C));
                addVec3(&sp2C, &self->pos, &sp80);
                copyVec3(&sp8C, &sp2C);
            }
            em_move_vec_clr(self);
            temp_f31_2 = get_em_base_scale(self);
            subVec3(&sp8, &sp8C, &self->pos);
            vec3_div(&sp14, &sp8, 30.0f);
            vec3_scale(&sp20, &sp14, temp_f31_2);
            copyVec3(&self->offset_0x30C.vec_0x310, &sp20);
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->timer_0x020 = (s32) (s16) (calcVecAng2(&sp8C, &self->vec_0x36C) - self->field_0x1C0);
            return;
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 3);
            if ((s32) mode == 0) {
                em_mot_set(self, 0xCE, 0, 0);
            } else {
                em_mot_set(self, 0xD0, 0, 0);
            }
            addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
            return;
        }
        break;
    case 3:
        if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
            em_turn_in_window(self, 0.0f, 30.0f, self->timer_0x020);
            addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        }
        if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
            em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * (0.2f * get_em_base_scale(self))) / 360.0f)));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_walk(self);
        }
        break;
    }
}

/* 0x801CF648 - the `state_sub` dispatcher over this range's 0x801CE99C.. band (10 ways). */
void em005_act4_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em005_act4_motion2A(self); break;
    case 1: em005_act4_motion2E(self); break;
    case 2: em005_act4_motion1E(self); break;
    case 3: em005_act4_land(self); break;
    case 4: em005_act4_turn(self); break;
    case 5: em005_act4_approach(self); break;
    case 6: em005_act4_leap(self, 0); break;
    case 7: em005_act4_leap(self, 1); break;
    case 8: em005_act4_charge(self, 0); break;
    case 9: em005_act4_charge(self, 1); break;
    }
}

/* 0x801CF6A8 - the mode-selected "hold the part pair" action: state 0 arms one of four motion/part
 * sets, state 1 closes on `em_mot_end_ck`. */
void em005_act7_swing(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_mot_set(self, 0xDC, 4, 0);
            em_hit_window_set(self, 0, 1, 8);
            em_hit_window_set(self, 1, 0x12, 0x10);
            break;
        case 1:
            em_mot_set(self, 0xDD, 4, 0);
            em_hit_window_set(self, 0, 2, 8);
            em_hit_window_set(self, 1, 0x13, 0x10);
            break;
        case 2:
            em_mot_set(self, 0xDC, 4, 0);
            em_hit_window_set(self, 0, 0x1E, 8);
            em_hit_window_set(self, 1, 0x12, 0x10);
            break;
        case 3:
            em_mot_set(self, 0xDD, 4, 0);
            em_hit_window_set(self, 0, 0x1F, 8);
            em_hit_window_set(self, 1, 0x13, 0x10);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act7_fireball(struct _ENEMY_WORK* self, u8 mode) {
    EmSpawnRec sp8;
    em_spawn_rec_init(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_stamina_add(self, -10);
        em_mot_set(self, 0xE4, 4, 0);
        if (mode != 1) {
            em_mot_speed_set(self, 1.4f);
            return;
        }
        break;
    case 1:
        if ((mode != 1) && (em_frame_check(self, 0, 120.0f, 0.0f) == 1)) {
            em_mot_speed_set(self, 1.0f);
        }
        if (em_frame_check(self, 3, 0.0f, 28.0f) == 1) {
            em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * (1.0714285f * get_em_base_scale(self))) / 360.0f)));
        }
        if ((em_frame_check(self, 3, 2.0f, 64.0f) == 1) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            eft007_part_set(self, 0x36);
        }
        switch ((s32) mode) {
        case 0:
            if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0xA00, 0);
                eft007_part_spawn(self, 0x37, sp8.field_0x12, sp8.field_0x14);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0xA00, 0);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &sp8, 0x19);
            }
            break;
        case 1:
            if (em_frame_check(self, 0, 104.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x800, 0);
                eft007_part_spawn(self, 0x3C, sp8.field_0x12, sp8.field_0x14);
            }
            break;
        case 2:
            if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x500, 0);
                eft007_part_spawn(self, 0x37, sp8.field_0x12, sp8.field_0x14);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x500, 0);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &sp8, 0x19);
            }
            break;
        case 3:
            if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x200, 0);
                eft007_part_spawn(self, 0x37, sp8.field_0x12, sp8.field_0x14);
            }
            if (em_frame_check(self, 0, 106.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x200, 0);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &sp8, 0x19);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801CFBF4 - the mode-paired "arm and wait" action: state 0 arms 0xDE/0xE1 and latches the aim
 * angle into `timer_0x020`, state 1 turns by it and hands over to the mode's band function. */
void em005_act7_turn(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_mot_set(self, 0xDE, 2, 0);
            break;
        case 1:
            em_mot_set(self, 0xE1, 2, 0);
            break;
        }
        self->timer_0x020 = (s16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        break;
    case 1:
        em_turn_in_window(self, 20.0f, 40.0f, self->timer_0x020);
        if (em_frame_check(self, 1, 72.0f, 0.0f) == 1) {
            self->state++;
            switch (mode) {
            case 0:
            case 2:
                em005_arm_act7_5(self);
                break;
            case 1:
            case 3:
                em005_arm_act7_6(self);
                break;
            }
        }
        break;
    }
}

/* 0x801CFD28 - `em005_act7_turn`'s timer-based sibling (the same mode pair with `em_mot_set_ck`). */
void em005_act7_motionDE(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        switch (mode) {
        case 0:
            em_mot_set_ck(self, 0xDE, 0, 0x48);
            break;
        case 1:
            em_mot_set_ck(self, 0xE1, 0, 0x48);
            break;
        }
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            switch (mode) {
            case 0:
                em005_arm_act7_5(self);
                break;
            case 1:
                em005_arm_act7_6(self);
                break;
            }
        }
        break;
    }
}

void em005_act7_jump(struct _ENEMY_WORK* self, u8 mode, u8 sub) {
    f32 var_f29;
    f32 var_f31;
    if ((s32) sub == 0) {
        var_f29 = 0.8f;
        var_f31 = 2.5f;
    } else {
        var_f29 = 0.6f;
        var_f31 = 3.3f;
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_stamina_add(self, -10);
        switch ((s32) mode) {
        case 0:
        case 2:
        case 4:
            em_mot_set(self, 0xDE, 2, 0);
            break;
        case 1:
        case 3:
        case 5:
            em_mot_set(self, 0xE1, 2, 0);
            break;
        }
        self->timer_0x020 = (s32) (s16) (calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        return;
    case 1:
        em_turn_in_window(self, 20.0f, 40.0f, self->timer_0x020);
        if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            switch ((s32) mode) {
            case 0:
            case 2:
            case 4:
                em_mot_set(self, 0xDF, 4, 0);
                em_hit_window_set_default(self, 0, 5);
                em_hit_window_set_default(self, 1, 7);
                break;
            case 1:
            case 3:
            case 5:
                em_mot_set(self, 0xE2, 4, 0);
                em_hit_window_set_default(self, 0, 6);
                em_hit_window_set_default(self, 1, 7);
                break;
            }
            em_move_vec2_clr(self);
            return;
        }
        break;
    case 2:
        self->offset_0x30C.vec_0x310.z = var_f29 * (em_key_curve_eval(self, &em005_jump_curve) * get_em_chg_scale(self));
        switch ((s32) mode) {
        case 0:
            em_turn_in_window(self, 12.0f, 22.0f, (s32) (u16) (s32) (0.5f + ((65536.0f * (10.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                self->state = 5;
                em_mot_set(self, 0xE0, 2, 0);
                return;
            }
            break;
        case 1:
            em_turn_in_window(self, 12.0f, 22.0f, -(s32) (u16) (s32) (0.5f + ((65536.0f * (10.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                self->state = 5;
                em_mot_set(self, 0xE3, 2, 0);
                return;
            }
            break;
        case 2:
        case 4:
            em_turn_in_window(self, 12.0f, 22.0f, (s32) (u16) (s32) (0.5f + ((65536.0f * (10.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_mot_set(self, 0xE2, 0, 0);
                em_hit_window_set_default(self, 0, 6);
                em_hit_window_set_default(self, 1, 7);
                return;
            }
            break;
        case 3:
        case 5:
            em_turn_in_window(self, 12.0f, 22.0f, -(s32) (u16) (s32) (0.5f + ((65536.0f * (10.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_mot_set(self, 0xDF, 0, 0);
                em_hit_window_set_default(self, 0, 5);
                em_hit_window_set_default(self, 1, 7);
                return;
            }
            break;
        }
        break;
    case 3:
        self->offset_0x30C.vec_0x310.z = var_f29 * (em_key_curve_eval(self, &em005_jump_curve) * get_em_chg_scale(self));
        switch ((s32) mode) {
        case 0:
        case 2:
            em_turn_in_window(self, 12.0f, 22.0f, -(s32) (u16) (s32) (0.5f + ((65536.0f * (20.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                self->state = 5;
                em_mot_set(self, 0xE3, 2, 0);
                return;
            }
            break;
        case 1:
        case 3:
            em_turn_in_window(self, 12.0f, 22.0f, (s32) (u16) (s32) (0.5f + ((65536.0f * (20.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                self->state = 5;
                em_mot_set(self, 0xE0, 2, 0);
                return;
            }
            break;
        case 4:
            em_turn_in_window(self, 12.0f, 22.0f, -(s32) (u16) (s32) (0.5f + ((65536.0f * (20.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_mot_set(self, 0xDF, 0, 0);
                em_hit_window_set_default(self, 0, 5);
                em_hit_window_set_default(self, 1, 7);
                return;
            }
            break;
        case 5:
            em_turn_in_window(self, 12.0f, 22.0f, (s32) (u16) (s32) (0.5f + ((65536.0f * (20.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_mot_end_ck(self) == 1) {
                self->state++;
                em_mot_set(self, 0xE2, 0, 0);
                em_hit_window_set_default(self, 0, 6);
                em_hit_window_set_default(self, 1, 7);
                return;
            }
            break;
        }
        break;
    case 4:
        self->offset_0x30C.vec_0x310.z = var_f29 * (em_key_curve_eval(self, &em005_jump_curve) * get_em_chg_scale(self));
        switch ((s32) mode) {
        case 0:
        case 2:
        case 4:
            em_turn_in_window(self, 12.0f, 22.0f, (s32) (u16) (s32) (0.5f + ((65536.0f * (20.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set(self, 0xE0, 2, 0);
                return;
            }
            break;
        case 1:
        case 3:
        case 5:
            em_turn_in_window(self, 12.0f, 22.0f, -(s32) (u16) (s32) (0.5f + ((65536.0f * (20.0f * var_f31)) / 360.0f)));
            em_move_offset_rot_apply(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 60.0f, 0.0f) == 1) {
                self->state++;
                em_mot_set(self, 0xE3, 2, 0);
                return;
            }
            break;
        }
        break;
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D076C - the two-frame-window action: state 0 arms the part pair, state 1 opens the 1/2
 * windows (the second one turns the work record) and closes. */
void em005_act7_bite(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xD4, 4, 0);
        em_hit_window_set(self, 0, 3, 8);
        em_hit_window_set(self, 1, 4, 0x18);
        break;
    case 1:
        if (em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 1);
            em_hit_window_set(self, 1, 0x10, 0x10);
        }
        if (em_frame_check(self, 2, 22.0f, 0.0f) == 1) {
            u16 angle = (u16)(s32)((65536.0f * (0.45454547f * get_em_base_scale(self)))
            / 360.0f
            + 0.5f);
            em_turn_to_target(self, angle);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D08A8 - lean over and slide: state 0 latches the aim angle (clamped to 0x4000), state 1 runs three
 * frame windows, state 2 closes. */
void em005_act7_lunge(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0: {
        u32 diff;
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0xD6, 6, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = 37.037037f * get_em_base_scale(self);
        self->field_0x318 =
            self->field_0x318 * em_target_dist_ratio_get(self, 1.7f, 0.4f, 3000.0f);
        diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff >= 0x8000) {
            diff = 0x10000 - diff;
        }
        self->timer_0x020 = (s16)diff;
        if (self->timer_0x020 > 0x4000) {
            self->timer_0x020 = 0x4000;
        }
        break;
    }
    case 1:
        if (em_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x0D, 0x0B);
        }
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            f32 scaled = (f32)self->timer_0x020 * 360.0f / 65536.0f;
            u16 angle = (u16)(s32)((65536.0f
            * (scaled / 30.0f * get_em_base_scale(self)))
            / 360.0f
            + 0.5f);
            em_turn_to_target(self, angle);
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD7, 0, 0);
            self->field_0x324 = -self->field_0x318 / 30.0f * get_em_base_scale(self);
            em_hit_window_set(self, 1, 0x19, 0x10);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 0.5f);
        }
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 < 0.0f) {
                self->field_0x318 = 0.0f;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act7_flip(struct _ENEMY_WORK* self, u8 mode) {
    struct EmRotWord3 rot;
    f32 temp_f31;
    u32 var_r3;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_stamina_add(self, -10);
        em_mot_set(self, 0xDA, 6, 0);
        em_move_vec2_clr(self);
        em_height_set(self, 0.0f);
        var_r3 = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (var_r3 >= 0x8000) {
            var_r3 = 0x10000 - var_r3;
        }
        self->timer_0x020 = (s32) (s16) var_r3;
        return;
    case 1:
        if (em_frame_check(self, 0, 94.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xA, 0xB);
        }
        if (em_frame_check(self, 3, 0.0f, 38.0f) == 1) {
            temp_f31 = (360.0f * (f32) self->timer_0x020) / 65536.0f;
            em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * ((temp_f31 / 38.0f) * get_em_base_scale(self))) / 360.0f)));
        }
        em_height_add(self, em_key_curve_eval(self, &em005_flip_rise_curve));
        self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_drop_curve);
        self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_fwd_curve);
        self->offset_0x30C.vec_0x310.z *= em_target_dist_ratio_get(self, 2.0f, 0.4f, 2000.0f);
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            if ((s32) mode == 0) {
                em_mot_set(self, 0xDB, 0, 0);
                em_hit_window_set(self, 1, 0xB, 0x18);
            } else {
                em_mot_set(self, 0xF6, 0, 0);
                em_hit_window_set(self, 1, 0x1C, 0x18);
            }
            em_height_set(self, 0.0f);
            self->timer_0x020 = (s32) (s16) self->field_0x1C0;
            return;
        }
        break;
    case 2:
        if ((s32) mode == 0) {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_set(self, 0, 0xC, 0x10);
            }
            em_turn_in_window(self, 0.0f, 30.0f, 0x8000);
            if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_kick_rise_curve);
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_kick_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        } else {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_clear(self, 0);
                em_drop_item_spawn(self, &self->pos, 0x64);
            }
            em_turn_in_window(self, 0.0f, 62.0f, 0x10000);
            if (em_frame_check(self, 3, 0.0f, 62.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_end_rise_curve) / 3.0f;
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_end_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D0FE0 - the two-mode "arm the part pair and turn" action. */
void em005_act7_swing2(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if (mode == 0) {
            em_mot_set(self, 0xD5, 4, 0);
            em_hit_window_set(self, 0, 8, 8);
            em_hit_window_set(self, 1, 9, 0x18);
        } else {
            em_mot_set(self, 0xD9, 4, 0);
            em_hit_window_set(self, 0, 0x11, 8);
            em_hit_window_set(self, 1, 0x24, 0x18);
        }
        break;
    case 1:
        if (mode == 0) {
            em_turn_in_window(self, 50.0f, 80.0f, 0x4000);
        } else {
            em_turn_in_window(self, 50.0f, 80.0f, -0x4000);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act7_flip_kick(struct _ENEMY_WORK* self, u8 mode) {
    struct EmRotWord3 rot;
    f32 temp_f31;
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_stamina_add(self, -10);
        em_mot_set(self, 0xE7, 6, 8);
        em_move_vec2_clr(self);
        em_busy_timer_reset(self);
        return;
    case 1:
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
        if (em_frame_check(self, 3, 0.0f, 14.0f) == 1) {
            self->offset_0x30C.vec_0x310.z = -21.428572f * get_em_base_scale(self);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_frame_check(self, 1, 28.0f, 0.0f) == 1) {
            self->state++;
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = 40.0f;
            self->offset_0x30C.vec_0x310.z = 60.0f * get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.z *= em_target_dist_ratio_get(self, 2.0f, 0.5f, 2000.0f);
            self->vec_0x31C.y = -10.0f;
            self->timer_0x020 = 0;
            em_hit_window_set(self, 0, 0xA, 0xB);
        }
        break;
    case 2:
        self->timer_0x020++;
        temp_f31 = (f32) self->timer_0x020 * get_em_base_scale(self);
        if (temp_f31 < 8.0f) {
            em_busy_timer_reset(self);
            em_frame_flag_set(self);
        }
        if ((temp_f31 >= 4.0f) && (temp_f31 < 14.0f)) {
            em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * (4.5f * get_em_base_scale(self))) / 360.0f)));
        }
        em_move_offset_step(self, &self->field_0x1BC);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            if ((s32) mode == 0) {
                em_mot_set(self, 0xDB, 0, 0);
                em_hit_window_set(self, 1, 0xB, 0x18);
            } else {
                em_mot_set(self, 0xF6, 0, 0);
                em_hit_window_set(self, 1, 0x1C, 0x18);
            }
            self->timer_0x020 = (s32) (s16) self->field_0x1C0;
            return;
        }
        break;
    case 3:
        if ((s32) mode == 0) {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_set(self, 0, 0xC, 0x10);
            }
            em_turn_in_window(self, 0.0f, 30.0f, 0x8000);
            if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_kick_rise_curve);
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_kick_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        } else {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_clear(self, 0);
            }
            em_turn_in_window(self, 0.0f, 62.0f, 0x10000);
            if (em_frame_check(self, 3, 0.0f, 62.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_end_rise_curve) / 3.0f;
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_end_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act7_rush(struct _ENEMY_WORK* self, u8 mode, u8 sub) {
    f32 temp_f31;
    u32 var_r3;
    u32 temp_r3;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 8, 4, 0);
        switch ((s32) mode) {
        case 0:
            em_approach_start(self, 100.0f, 0);
            break;
        case 1:
            em_approach_start(self, 0.0f, 0);
            break;
        case 2:
            em_approach_start(self, 500.0f, 0);
            break;
        case 3:
            em_approach_start(self, 0.0f, 0);
            break;
        }
        var_r3 = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (var_r3 >= 0x8000) {
            var_r3 = 0x10000 - var_r3;
        }
        self->timer_0x020 = (s32) (s16) var_r3;
        if ((s16) var_r3 > 0x3000) {
            self->timer_0x020 = 0x3000;
            return;
        }
        break;
    case 1:
        if (em_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xE, 3);
        }
        if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
            temp_f31 = (360.0f * (f32) self->timer_0x020) / 65536.0f;
            em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * ((temp_f31 / 30.0f) * get_em_base_scale(self))) / 360.0f)));
        }
        temp_r3 = em_approach_step(self, 0, 0);
        if ((mode == 3) && (temp_r3 == 1)) {
            em_action_finish(self);
            return;
        }
        if (em_frame_check(self, 1, 84.0f, 0.0f) == 1) {
            self->state++;
            break;
        }
        break;
    case 2:
        if (em_approach_step(self, 0, 0) == 1) {
            switch ((s32) mode) {
            case 0:
            case 2:
                self->state++;
                em_mot_set(self, 9, 4, 0);
                return;
            case 1:
            case 3:
                em_action_finish(self);
                return;
            }
        }
        break;
    case 3:
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if ((sub == 1) && (em_frame_check(self, 0, 40.0f, 0.0f) == 1)) {
            em_mot_speed_set(self, 0.7f);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act7_flip_aim(struct _ENEMY_WORK* self, u8 mode, u8 sub) {
    nw4r::math::VEC3 sp34;
    struct EmRotWord3 rot;
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    u32 sp8;
    VEC3_ctor(&sp34);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_stamina_add(self, -10);
        em_mot_set(self, 0xE8, 6, 0);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = 15.0f * get_em_base_scale(self);
        self->vec_0x31C.z = 3.0f * get_em_base_scale(self);
        return;
    case 1:
        subVec3(&sp1C, &self->vec_0x36C, &self->pos);
        copyVec3(&sp34, &sp1C);
        calcVecAngXY(&sp34, &spC, &sp8);
        if (spC < 0x38EU) {
            spC = 0x38E;
        } else if (spC > 0x3C72U) {
            spC = 0x3C72;
        }
        copyVec3(&sp34, &self->offset_0x30C.vec_0x310);
        rotVecX(&sp34, spC);
        rotVecY(&sp34, self->field_0x1C0);
        em_hit_arm_set(self);
        self->field_0x1BC = (u32) em_angle_approach((u16) spC, (u16) self->field_0x1BC, 0);
        addVec3To(&self->pos, &sp34);
        self->offset_0x30C.vec_0x310.z += self->vec_0x31C.z;
        if (self->offset_0x30C.vec_0x310.z > (100.0f * get_em_base_scale(self))) {
            self->offset_0x30C.vec_0x310.z = 100.0f * get_em_base_scale(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xE7, 0, 0x3A);
            em_hit_window_set(self, 0, 0xA, 0xB);
            return;
        }
        break;
    case 2:
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp34, &sp10);
        calcVecAngXY(&sp34, &spC, &sp8);
        if (spC < 0x38EU) {
            spC = 0x38E;
        } else if (spC > 0x3C72U) {
            spC = 0x3C72;
        }
        copyVec3(&sp34, &self->offset_0x30C.vec_0x310);
        rotVecX(&sp34, spC);
        rotVecY(&sp34, self->field_0x1C0);
        em_hit_arm_set(self);
        self->field_0x1BC = (u32) em_angle_approach((u16) spC, (u16) self->field_0x1BC, 0x100);
        addVec3To(&self->pos, &sp34);
        self->offset_0x30C.vec_0x310.z += self->vec_0x31C.z;
        if (self->offset_0x30C.vec_0x310.z > (100.0f * get_em_base_scale(self))) {
            self->offset_0x30C.vec_0x310.z = 100.0f * get_em_base_scale(self);
        }
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            if ((s32) mode == 0) {
                em_mot_set(self, 0xDB, 0, 0);
                em_hit_window_set(self, 1, 0xB, 0x18);
            } else {
                em_mot_set(self, 0xF6, 0, 0);
                em_hit_window_set(self, 1, 0x1C, 0x18);
            }
            self->timer_0x020 = (s32) (s16) self->field_0x1C0;
            self->field_0x1BC = 0;
            return;
        }
        break;
    case 3:
        if ((s32) mode == 0) {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_set(self, 0, 0xC, 0x10);
            }
            em_turn_in_window(self, 0.0f, 30.0f, 0x8000);
            if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_kick_rise_curve);
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_kick_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        } else {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_clear(self, 0);
            }
            em_turn_in_window(self, 0.0f, 62.0f, 0x10000);
            if (em_frame_check(self, 3, 0.0f, 62.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_end_rise_curve) / 3.0f;
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_end_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act7_fire_spread(struct _ENEMY_WORK* self, u8 mode) {
    EmSpawnRec sp8;
    em_spawn_rec_init(&sp8);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_stamina_add(self, -10);
        em_mot_set(self, 0xD8, 6, 0);
        em_move_vec2_clr(self);
        return;
    case 1:
        self->offset_0x30C.vec_0x310.z = 0.6f * em_key_curve_eval(self, &em005_fire_spread_curve);
        em_move_offset_step(self, &self->field_0x1BC);
        if ((em_frame_check(self, 3, 2.0f, 48.0f) == 1) && ((s32) (system_w.field_0x0c & 7) == 0)) {
            eft007_part_set(self, 0x36);
        }
        switch ((s32) mode) {
        case 0:
            if (em_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x2000, 0);
                eft007_part_spawn(self, 0x37, sp8.field_0x12, sp8.field_0x14);
            }
            if (em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x2000, 0);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &sp8, 0x19);
            }
            break;
        case 1:
            if (em_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x2000, 0);
                eft007_part_spawn(self, 0x3C, sp8.field_0x12, sp8.field_0x14);
            }
            break;
        case 2:
            if (em_frame_check(self, 3, 0.0f, 60.0f) == 1) {
                em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * (2.0f * get_em_base_scale(self))) / 360.0f)));
            }
            if (em_frame_check(self, 0, 82.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x2000, 0);
                eft007_part_spawn(self, 0x37, sp8.field_0x12, sp8.field_0x14);
            }
            if (em_frame_check(self, 0, 84.0f, 0.0f) == 1) {
                em005_spawn_rec_fill(&sp8, 0, 0x2000, 0);
                ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &sp8, 0x19);
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xEF, 0, 0);
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

/* The side charge into the rush: the side run of the charge, then the blended rush toward the target (hit window from
 * frame 54) and, unless `sub` is 0, the closing motion. */
void em005_act7_charge(struct _ENEMY_WORK* self, u8 mode, u8 sub) {
    nw4r::math::VEC3 point;
    nw4r::math::VEC3 offset;
    u32 angle;

    VEC3_ctor(&point);
    VEC3_ctor(&offset);
    switch (self->state) {
    case 0:
        em_move_mode_set(self, 0);
        em_stamina_add(self, -10);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000) {
            angle = 0x10000 - angle;
        }
        if (angle < 0x4000) {
            self->state = 2;
            switch (mode) {
            case 0:
                em_mot_set(self, 0xC9, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x4000 + self->bits_0x1EC % 0x2000;
                break;
            case 1:
                em_mot_set(self, 0xCA, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) - 0x4000 - self->bits_0x1EC % 0x2000;
                break;
            }
            setVector3(&offset, 0.0f, 0.0f, 600.0f);
            rotVecY(&offset, angle);
            {
                nw4r::math::VEC3 sum;
                addVec3(&sum, &self->vec_0x36C, &offset);
                copyVec3(&point, &sum);
            }
            if (calcDistanceSqXZ(&point, &self->pos) > 9000000.0f) {
                setVector3(&offset, 0.0f, 0.0f, 3000.0f);
                rotVecY(&offset, calcVecAng2(&self->pos, &point));
                {
                    nw4r::math::VEC3 sum;
                    addVec3(&sum, &self->pos, &offset);
                    copyVec3(&point, &sum);
                }
            }
            em_move_vec_clr(self);
            {
                f32 scale = get_em_base_scale(self);
                nw4r::math::VEC3 move;
                nw4r::math::VEC3 step;
                nw4r::math::VEC3 diff;
                subVec3(&diff, &point, &self->pos);
                vec3_div(&step, &diff, 22.0f);
                vec3_scale(&move, &step, scale);
                copyVec3(&self->offset_0x30C.vec_0x310, &move);
            }
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->timer_0x020 = (s16)(calcVecAng2(&point, &self->vec_0x36C) - self->field_0x1C0);
            break;
        }
        self->state++;
        em_turn_seq_start(self, (void*)em005_turn_tbl_wide, 0, 1, 0);
        break;
    case 1:
        if (em_turn_seq_step(self, (void*)em005_turn_tbl_wide) == 1) {
            self->state++;
            switch (mode) {
            case 0:
                em_mot_set(self, 0xC9, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) + 0x2000 + self->bits_0x1EC % 0x4000;
                break;
            case 1:
                em_mot_set(self, 0xCA, 4, 0);
                angle = calcVecAng2(&self->pos, &self->vec_0x36C) - 0x6000 + self->bits_0x1EC % 0x4000;
                break;
            }
            setVector3(&offset, 0.0f, 0.0f, 600.0f);
            rotVecY(&offset, angle);
            {
                nw4r::math::VEC3 sum;
                addVec3(&sum, &self->vec_0x36C, &offset);
                copyVec3(&point, &sum);
            }
            if (calcDistanceSqXZ(&point, &self->pos) > 9000000.0f) {
                setVector3(&offset, 0.0f, 0.0f, 3000.0f);
                rotVecY(&offset, calcVecAng2(&self->pos, &point));
                {
                    nw4r::math::VEC3 sum;
                    addVec3(&sum, &self->pos, &offset);
                    copyVec3(&point, &sum);
                }
            }
            em_move_vec_clr(self);
            {
                f32 scale = get_em_base_scale(self);
                nw4r::math::VEC3 move;
                nw4r::math::VEC3 step;
                nw4r::math::VEC3 diff;
                subVec3(&diff, &point, &self->pos);
                vec3_div(&step, &diff, 22.0f);
                vec3_scale(&move, &step, scale);
                copyVec3(&self->offset_0x30C.vec_0x310, &move);
            }
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->timer_0x020 = (s16)(calcVecAng2(&point, &self->vec_0x36C) - self->field_0x1C0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            switch (mode) {
            case 0:
                em_mot_set(self, 0xF4, 0, 0);
                break;
            case 1:
                em_mot_set(self, 0xF5, 0, 0);
                break;
            }
            addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        }
        break;
    case 3:
        em_turn_in_window(self, 0.0f, 22.0f, self->timer_0x020);
        addVec3To(&self->pos, &self->offset_0x30C.vec_0x310);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            switch (mode) {
            case 0:
                em_mot_set(self, 0xF0, 0, 0);
                em_hit_window_set_default(self, 0, 0x1A);
                break;
            case 1:
                em_mot_set(self, 0xF1, 0, 0);
                em_hit_window_set_default(self, 0, 0x1B);
                break;
            }
            angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (angle >= 0x8000) {
                angle = 0x10000 - angle;
            }
            self->timer_0x020 = (s16)angle;
            if (self->timer_0x020 > 0x2000) {
                self->timer_0x020 = 0x2000;
            }
        }
        break;
    case 4:
        if (em_frame_check(self, 3, 0.0f, 26.0f) == 1) {
            f32 deg = (360.0f * (f32)self->timer_0x020) / 65536.0f;
            em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * ((deg / 24.0f) * get_em_base_scale(self))) / 360.0f));
        }
        if (em_frame_check(self, 1, 32.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set_blend(self, 8, 0xC, 0x2A, 1);
            em_approach_start(self, 500.0f, 0);
            em_hit_window_clear_all(self);
        }
        break;
    case 5:
        if (em_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xE, 3);
        }
        em_approach_step(self, 0, 0x80);
        if (em_frame_check(self, 1, 84.0f, 0.0f) == 1) {
            self->state++;
        }
        break;
    case 6:
        if (em_approach_step(self, 0, 0) == 1) {
            if (sub == 0) {
                em_action_finish(self);
            } else {
                self->state++;
                em_mot_set(self, 9, 4, 0);
            }
        }
        break;
    case 7:
        if (em_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x801D290C - the `state_sub` dispatcher over this range's upper action band (48 ways; the table is
 * `jumptable_805B553C`).  Each arm is a tail call, so each case is a `return`. */
void em005_act7_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: em005_act7_swing(self, 0); break;
    case 1: em005_act7_swing(self, 1); break;
    case 2: em005_act7_fireball(self, 0); break;
    case 3: em005_act7_turn(self, 0); break;
    case 4: em005_act7_turn(self, 1); break;
    case 5: em005_act7_motionDE(self, 0); break;
    case 6: em005_act7_motionDE(self, 1); break;
    case 7: em005_act7_jump(self, 0, 0); break;
    case 8: em005_act7_jump(self, 1, 0); break;
    case 9: em005_act7_jump(self, 2, 0); break;
    case 10: em005_act7_jump(self, 3, 0); break;
    case 11: em005_act7_jump(self, 4, 0); break;
    case 12: em005_act7_jump(self, 5, 0); break;
    case 13: em005_act7_bite(self, 0); break;
    case 14: em005_act7_lunge(self); break;
    case 15: em005_act7_flip(self, 0); break;
    case 16: em005_act7_swing2(self, 0); break;
    case 17: em005_act7_flip_kick(self, 0); break;
    case 18: em005_act7_rush(self, 0, 0); break;
    case 19: em005_act7_rush(self, 1, 0); break;
    case 20: em005_act7_flip_aim(self, 0, 0); break;
    case 21: em005_act7_fire_spread(self, 0); break;
    case 22: em005_act7_jump(self, 0, 1); break;
    case 23: em005_act7_jump(self, 1, 1); break;
    case 24: em005_act7_jump(self, 2, 1); break;
    case 25: em005_act7_jump(self, 3, 1); break;
    case 26: em005_act7_jump(self, 4, 1); break;
    case 27: em005_act7_jump(self, 5, 1); break;
    case 28: em005_act7_flip(self, 1); break;
    case 29: em005_act7_flip_kick(self, 1); break;
    case 30: em005_act7_flip_aim(self, 1, 0); break;
    case 31: em005_act7_fireball(self, 1); break;
    case 32: em005_act7_fire_spread(self, 1); break;
    case 33: em005_act7_swing(self, 2); break;
    case 34: em005_act7_swing(self, 3); break;
    case 35: em005_act7_charge(self, 0, 0); break;
    case 36: em005_act7_charge(self, 1, 0); break;
    case 37: em005_act7_charge(self, 0, 1); break;
    case 38: em005_act7_charge(self, 1, 1); break;
    case 39: em005_act7_rush(self, 2, 0); break;
    case 40: em005_act7_bite(self, 1); break;
    case 41: em005_act7_rush(self, 0, 1); break;
    case 42: em005_act7_rush(self, 2, 1); break;
    case 43: em005_act7_swing2(self, 1); break;
    case 44: em005_act7_fireball(self, 2); break;
    case 45: em005_act7_fireball(self, 3); break;
    case 46: em005_act7_fire_spread(self, 2); break;
    case 47: em005_act7_rush(self, 3, 0); break;
    }
}

/* The per-motion sound tables action 10 plays (`em_se_tbl_play`), each with its records, and the shift offsets
 * (`.sdata`) the shifted records point at. */
EmSeRecord em005_se_tbl_mot18_rec0 = {0x64, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot18[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot18_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot1E_rec0 = {0x65, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot1E[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot1E_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot17_rec0 = {0x83, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot17[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot17_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot58_rec0 = {0x84, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot58[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot58_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot38_rec0 = {0x84, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot38_rec1 = {0xD4, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot38[3] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot38_rec0},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot38_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot19_rec0 = {0x66, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot19_rec1 = {0x68, 0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot19_rec2 = {0x6A, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot19[5] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot19_rec0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot19_rec1},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot19_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot1A_rec0 = {0x67, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot1A_rec1 = {0x69, 0, 0, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot1A_rec2 = {0x6B, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot1A[5] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot1A_rec0},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot1A_rec1},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot1A_rec2},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot23_rec0 = {0x70, 0, 4, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em005_vec_pair_0.pos};
EmSeOffset em005_se_shift_tail = {-50.0f, -10.0f};
EmSeRecord em005_se_tbl_mot23_rec1 = {0x0, 0, 0, 0, 0, {0, 2, 1, 0}, &em005_se_shift_tail};
EmSeRecord em005_se_tbl_mot23_rec2 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot23_rec3 = {0x69, 0, 4, 0, 0, {0, 4, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot23_rec4 = {0x6B, 0, 8, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot23[11] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x12C)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot23_rec0},
    {0x01, {0, 0, 0}, &em005_se_tbl_mot23_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot23_rec2},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot23_rec3},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot23_rec4},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot7A_rec0 = {0x74, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot7A[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7A_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot7B_rec0 = {0x18, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot7B[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7B_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot9F_rec0 = {0x19, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot9F_rec1 = {0x7C, 0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot9F[3] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot9F_rec0},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot9F_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot7C_rec0 = {0x73, 0, 6, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot7C[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7C_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot7E_rec0 = {0x67, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot7E_rec1 = {0x69, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot7E_rec2 = {0x6B, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot7E[4] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7E_rec0},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7E_rec1},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7E_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot7F_rec0 = {0x66, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot7F_rec1 = {0x68, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot7F_rec2 = {0x6A, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot7F[4] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7F_rec0},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7F_rec1},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot7F_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot8D_rec0 = {0x7C, 0, 10, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot8D[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot8D_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot84_rec0 = {0x70, 0, 4, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em005_vec_pair_0.pos};
EmSeRecord em005_se_tbl_mot84_rec1 = {0x0, 0, 0, 0, 0, {0, 2, 1, 0}, &em005_se_shift_tail};
EmSeRecord em005_se_tbl_mot84_rec2 = {0x71, 0, 0, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em005_se_tbl_mot84_rec3 = {0x6B, 0, 18, 0, 1, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot84[9] = {
    {0x0B, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot84_rec0},
    {0x01, {0, 0, 0}, &em005_se_tbl_mot84_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x42)},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot84_rec2},
    {0x00, {0, 0, 0}, &em005_se_tbl_mot84_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_mot78_rec0 = {0x65, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_mot78[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_mot78_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motA8_rec0 = {0x84, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_motA8[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_motA8_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motB6_rec0 = {0x7F, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_motB6[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_motB6_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motB7_rec0 = {0x80, 0, 4, 0, 0, {2, 0, 0, 0}, 0};
EmSeEntry em005_se_tbl_motB7[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_motB7_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motB8_rec0 = {0x43, 0, 4, 4, 0, {0, 1, 2, 0}, (const EmSeOffset*)&em005_vec_pair_2.pos};
EmSeOffset em005_se_shift_head = {-5.0f, 60.0f};
EmSeRecord em005_se_tbl_motB8_rec1 = {0x1A, 0, 6, 0, 0, {0, 3, 4, 0}, &em005_se_shift_head};
EmSeRecord em005_se_tbl_motB8_rec2 = {0x2E, 0, 6, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_tbl_motB8_rec3 = {0x41, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_motB8[9] = {
    {0x0B, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em005_se_tbl_motB8_rec0},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x1F4)},
    {0x00, {0, 0, 0}, &em005_se_tbl_motB8_rec1},
    {0x04, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3)},
    {0x00, {0, 0, 0}, &em005_se_tbl_motB8_rec2},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em005_se_tbl_motB8_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motB9_rec0 = {0x82, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_motB9[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_motB9_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motBA_rec0 = {0x0, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em005_se_tbl_motBA[2] = {
    {0x01, {0, 0, 0}, &em005_se_tbl_motBA_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motBB_rec0 = {0x82, 0, 4, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em005_se_tbl_motBB[2] = {
    {0x00, {0, 0, 0}, &em005_se_tbl_motBB_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em005_se_tbl_motBF_rec0 = {0x84, 0, 2, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em005_se_tbl_motBF_rec1 = {0x73, 0, 6, 0, 0, {2, 0, 0, 0}, 0};
EmSeEntry em005_se_tbl_motBF[6] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em005_se_tbl_motBF_rec0},
    {0x06, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em005_se_tbl_motBF_rec1},
    {0xFE, {0, 0, 0}, 0},
};
EmSePosRec em005_se_tbl_motCA_rec0 = {60.0f, 100.0f, 0, 16384};
EmSePosRec em005_se_tbl_motCA_rec2 = {140.0f, 220.0f, 0, 16384};
EmSeRecord em005_se_tbl_motCA_rec1 = {0x78, 0, 0, 0, 0, {0, 8, 0, 0}, 0};
EmSeRecord em005_se_tbl_motCA_rec3 = {0x0, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_tbl_motCA[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x64)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em005_se_tbl_motCA_rec0},
    {0x00, {0, 0, 0}, &em005_se_tbl_motCA_rec1},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em005_se_tbl_motCA_rec2},
    {0x01, {0, 0, 0}, &em005_se_tbl_motCA_rec3},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801D2B10 - the 180-way `state_sub` dispatcher (0x17..0xCA): each case hands `em_se_tbl_play` its
 * `.data` cell and index; the default closes the action. */
void em005_act10_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17:
        em_se_tbl_play(self, em005_se_tbl_mot17, 0, 0x17);
        break;
    case 0x18:
        em_se_tbl_play(self, em005_se_tbl_mot18, 0, 0x18);
        break;
    case 0x19:
        em_se_tbl_play(self, em005_se_tbl_mot19, 0, 0x19);
        break;
    case 0x1A:
        em_se_tbl_play(self, em005_se_tbl_mot1A, 0, 0x1A);
        break;
    case 0x1B:
        em_se_tbl_play(self, em005_se_tbl_mot18, 0, 0x1B);
        break;
    case 0x1C:
        em_se_tbl_play(self, em005_se_tbl_mot19, 0, 0x1C);
        break;
    case 0x1D:
        em_se_tbl_play(self, em005_se_tbl_mot1A, 0, 0x1D);
        break;
    case 0x1E:
        em_se_tbl_play(self, em005_se_tbl_mot1E, 0, 0x1E);
        break;
    case 0x38:
        em_se_tbl_play(self, em005_se_tbl_mot38, 0, 0x38);
        break;
    case 0x58:
        em_se_tbl_play(self, em005_se_tbl_mot58, 0, 0x58);
        break;
    case 0x23:
        em_se_tbl_play(self, em005_se_tbl_mot23, 1, 0x23);
        break;
    case 0x7A:
        em_se_tbl_play(self, em005_se_tbl_mot7A, 0, 0x7A);
        break;
    case 0x7B:
        em_se_tbl_play(self, em005_se_tbl_mot7B, 0, 0x7B);
        break;
    case 0x9F:
        em_se_tbl_play(self, em005_se_tbl_mot9F, 0, 0x9F);
        break;
    case 0xA0:
        em_se_tbl_play(self, em005_se_tbl_mot9F, 0, 0xA0);
        break;
    case 0x7C:
        em_se_tbl_play(self, em005_se_tbl_mot7C, 0, 0x7C);
        break;
    case 0x8D:
        em_se_tbl_play(self, em005_se_tbl_mot8D, 0, 0x8D);
        break;
    case 0x7E:
        em_se_tbl_play(self, em005_se_tbl_mot7E, 0, 0x7E);
        break;
    case 0x7F:
        em_se_tbl_play(self, em005_se_tbl_mot7F, 0, 0x7F);
        break;
    case 0x8E:
        em_se_tbl_play(self, em005_se_tbl_mot8D, 0, 0x8E);
        break;
    case 0x78:
        em_se_tbl_play(self, em005_se_tbl_mot78, 0, 0x78);
        break;
    case 0x84:
        em_se_tbl_play(self, em005_se_tbl_mot84, 1, 0x84);
        break;
    case 0xA8:
        em_se_tbl_play(self, em005_se_tbl_motA8, 0, 0xA8);
        break;
    case 0xA9:
        em_se_tbl_play(self, em005_se_tbl_mot23, 1, 0xA9);
        break;
    case 0xB6:
        em_se_tbl_play(self, em005_se_tbl_motB6, 0, 0xB6);
        break;
    case 0xB7:
        em_se_tbl_play(self, em005_se_tbl_motB7, 0, 0xB7);
        break;
    case 0xB8:
        em_se_tbl_play(self, em005_se_tbl_motB8, 1, 0xB8);
        break;
    case 0xB9:
        em_se_tbl_play(self, em005_se_tbl_motB9, 0, 0xB9);
        break;
    case 0xBA:
        em_se_tbl_play(self, em005_se_tbl_motBA, 0, 0xBA);
        break;
    case 0xBB:
        em_se_tbl_play(self, em005_se_tbl_motBB, 0, 0xBB);
        break;
    case 0xBC:
        em_se_tbl_play(self, em005_se_tbl_motBB, 0, 0xBC);
        break;
    case 0xBF:
        em_se_tbl_play(self, em005_se_tbl_motBF, 0, 0xBF);
        break;
    case 0xC1:
        em_se_tbl_play(self, em005_se_tbl_mot8D, 0, 0xC1);
        break;
    case 0xCA:
        em_se_tbl_play(self, em005_se_tbl_motCA, 0, 0xCA);
        break;
    case 0xAF:
        em_se_tbl_play(self, em005_se_tbl_mot18, 0, 0xAF);
        break;
    case 0xB0:
        em_se_tbl_play(self, em005_se_tbl_mot23, 1, 0xB0);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* The sound tables action 11 plays (`em_se_tbl_play_alt`), each with its records. */
EmSeRecord em005_se_alt_tbl_mot00_rec0 = {0x7D, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_alt_tbl_mot00[3] = {
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot00_rec0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_alt_tbl_mot05_rec0 = {0x70, 0, 4, 0, 0, {0, 8, 2, 0}, (const EmSeOffset*)&em005_vec_pair_3.pos};
EmSeOffset em005_se_shift_tail2 = {-50.0f, -10.0f};
EmSeRecord em005_se_alt_tbl_mot05_rec1 = {0x0, 0, 0, 0, 0, {0, 2, 1, 0}, &em005_se_shift_tail2};
EmSeRecord em005_se_alt_tbl_mot05_rec2 = {0x71, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_alt_tbl_mot05_rec3 = {0x6B, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_alt_tbl_mot05_rec4 = {0x7D, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_alt_tbl_mot05[10] = {
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x12C)},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x28)},
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot05_rec0},
    {0x01, {0, 0, 0}, &em005_se_alt_tbl_mot05_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot05_rec2},
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot05_rec3},
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot05_rec4},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_alt_tbl_mot0F_rec0 = {0x82, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_alt_tbl_mot0F[3] = {
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot0F_rec0},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_alt_tbl_mot1A_rec0 = {0x74, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em005_se_alt_tbl_mot1A_rec1 = {0x18, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_alt_tbl_mot1A[3] = {
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot1A_rec0},
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot1A_rec1},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em005_se_alt_tbl_mot1C_rec0 = {0x82, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em005_se_alt_tbl_mot1C[2] = {
    {0x00, {0, 0, 0}, &em005_se_alt_tbl_mot1C_rec0},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801D2E0C - the `state_sub`-keyed file-row selector: five sub-states (plus the default) each
 * hand `em_se_tbl_play_alt` one of the band's `.data` rows and the row's own sub-state pair. */
void em005_act11_se_play(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: return em_se_tbl_play_alt(self, em005_se_alt_tbl_mot00, 0, 0);
    case 5: return em_se_tbl_play_alt(self, em005_se_alt_tbl_mot05, 1, 5);
    case 0x0F: return em_se_tbl_play_alt(self, em005_se_alt_tbl_mot0F, 0, 0x0F);
    case 0x1A: return em_se_tbl_play_alt(self, em005_se_alt_tbl_mot1A, 0, 0x1A);
    case 0x1C: return em_se_tbl_play_alt(self, em005_se_alt_tbl_mot1C, 0, 0x1C);
    default: return em_se_tbl_play_alt(self, em005_se_alt_tbl_mot00, 0, 0);
    }
}

/* 0x801D2EB4 - the tail call into `em005_act3_land(self, 0)` (the small thunk the unit above's
 * dispatcher uses). */
void em005_act12_land(struct _ENEMY_WORK* self) {
    return em005_act3_land(self, 0);
}

/* 0x801D2EBC - `state_sub == 0` gates the same thunk. */
void em005_act12_run(struct _ENEMY_WORK* self) {
    if (self->state_sub != 0) {
        return;
    }
    em005_act12_land(self);
}

/* 0x801D2ED0 - steps onto the area's offset point: state 0 picks the per-area offset, states 1/2 slide
 * there, state 3 lands (the part levels pick the landing sub-step). */
void em005_act13_fly_in(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    u32 have_spot;
    VEC3_ctor(&spot);
    have_spot = 0;
    switch (self->state) {
    case 0:
        if ((u8)stage_map_kind_get(self->field_0x1E0) == 4) {
            switch (self->area_no) {
            case 1:
                setVector3(&spot, -654.0f, 8000.0f, 1598.0f);
                have_spot = 1;
                break;
            case 2:
                setVector3(&spot, -2578.0f, 8000.0f, -1253.0f);
                have_spot = 1;
                break;
            case 3:
                setVector3(&spot, 950.0f, 8000.0f, -3978.0f);
                have_spot = 1;
                break;
            }
        }
        if (have_spot == 1) {
            self->state++;
            em_fall_height_get(self);
            em_fall_start(self);
            copyVec3(&self->action_0x328.vec_0x334, &self->vec_0x36C);
            copyVec3(&self->vec_0x36C, &spot);
            em_fly_start(self, (void*)em005_fly_tbl);
            em_approach_start(self, 0.0f, 0x19);
            em_fall_height_get(self);
            em_move_rot_ground_step(self, &self->field_0x1BC);
        } else {
            self->state = 3;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x1A, 0x0A, 0);
            em_dive_start(self);
            self->timer_0x020 = 0x12C;
        }
        break;
    case 1:
        if (em_approach_step(self, 0, 0) == 1) {
            self->state++;
            copyVec3(&self->vec_0x36C, &self->action_0x328.vec_0x334);
            em_mot_set(self, 0x1A, 0x0A, 0);
        } else {
            em_fly_step(self, 2, (void*)em005_fly_tbl);
            em_fall_height_get(self);
            em_move_rot_ground_step(self, &self->field_0x1BC);
        }
        break;
    case 2:
        if (em_turn_to_target(self, 0x200) == 1) {
            self->state++;
            em_dive_start(self);
            self->timer_0x020 = 0x12C;
        }
        break;
    case 3: {
        f32 gap = self->vec_0x36C.y - self->pos.y;
        s32 timer;
        if (gap < -2000.0f) {
            em_dive_step(self);
        } else if (gap > -1500.0f) {
            self->pos.y += 10.0f;
        }
        gap = self->vec_0x36C.y - self->pos.y;
        timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0
            || (em_turn_to_target(self, 0x180) == 1 && gap >= -2000.0f && gap <= -1500.0f)) {
            if (em_parts_damage_level_get(self, 2) >= 1
                && em_parts_damage_level_get(self, 3) >= 1) {
                em_state_set(self, 0x0D, 6);
            } else {
                em_state_set(self, 0x0D, 5);
            }
        }
        break;
    }
    }
}

void em005_act13_dive(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 angle_x;
    u32 angle_y;
    u32 var_r30;
    VEC3_ctor(&sp1C);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_blend(self, 0x20, 0xA, 0, 1);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = -20.0f;
        self->vec_0x31C.z = 0.5f;
        setVector3(&sp1C, 0.0f, 0.0f, 500.0f);
        angle_y = calcVecAng2(&self->vec_0x36C, &self->pos);
        rotVecY(&sp1C, angle_y);
        addVec3To(&self->vec_0x36C, &sp1C);
        return;
    case 1:
        em_hit_arm_set(self);
        em_fall_height_get(self);
        em_move_offset_step_update_v(self, &self->field_0x1BC);
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp1C, &sp10);
        calcVecAngXY(&sp1C, &angle_x, &angle_y);
        angle_x += 0x400;
        if (angle_x > 0x3000) {
            angle_x = 0x3000;
        }
        self->field_0x1BC = em_angle_approach((u16)angle_x, (u16)self->field_0x1BC, 0x100);
        self->field_0x1C0 = em_angle_approach((u16)angle_y, (u16)self->field_0x1C0, 0x40);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_vec_clr(self);
            self->offset_0x30C.vec_0x310.z = 80.0f;
            self->vec_0x31C.z = 0.5f;
            self->offset_0x30C.vec_0x310.y = -2.0f;
            em_mot_set(self, 0x1F, 4, 0);
            em_approach_start(self, -1500.0f, 0x12);
            return;
        }
        break;
    case 2:
        em_hit_arm_set(self);
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 0x2D, 6, 0);
            self->offset_0x30C.vec_0x310.y = -15.0f;
            self->vec_0x31C.z = -6.2f;
            em_fall_height_get(self);
            em_move_offset_step_update_v(self, &self->field_0x1BC);
            return;
        }
        em_fall_height_get(self);
        em_move_offset_step_update_v(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > 150.0f) {
            self->offset_0x30C.vec_0x310.z = 150.0f;
            return;
        }
        break;
    case 3:
        switch (self->state_0x006) {
        case 0:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
                self->state_0x006++;
                self->vec_0x31C.z = -6.8f;
            }
            break;
        case 1:
            em_fall_height_get(self);
            var_r30 = em_move_offset_step_update_v(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z < 0.0f) {
                self->offset_0x30C.vec_0x310.z = 0.0f;
            }
            break;
        }
        if (em_mot_end_ck(self) == 1) {
            if (var_r30 == 1U) {
                em_move_mode_set(self, 3);
                em_state_set(self, 0xD, 2);
                return;
            }
            em_action_finish_fall(self);
        }
        break;
    }
}

void em005_act13_lunge(struct _ENEMY_WORK* self) {
    f32 temp_f31;
    u32 var_r3;
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 3);
        em_mot_set(self, 0xD6, 6, 0);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = 27.777779f * get_em_base_scale(self);
        self->offset_0x30C.vec_0x310.z *= em_target_dist_ratio_get(self, 1.7f, 0.1f, 3000.0f);
        var_r3 = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (var_r3 >= 0x8000) {
            var_r3 = 0x10000 - var_r3;
        }
        self->timer_0x020 = (s32) (s16) var_r3;
        if ((s16) var_r3 > 0x4000) {
            self->timer_0x020 = 0x4000;
            return;
        }
        break;
    case 1:
        if (em_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x1D, 0xB);
        }
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            temp_f31 = (360.0f * (f32) self->timer_0x020) / 65536.0f;
            em_turn_to_target(self, (u32) (u16) (s32) (0.5f + ((65536.0f * ((temp_f31 / 30.0f) * get_em_base_scale(self))) / 360.0f)));
        }
        em_move_offset_rot_apply(self, &self->field_0x1BC);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xD7, 0, 0);
            self->vec_0x31C.z = (-self->offset_0x30C.vec_0x310.z / 30.0f) * get_em_base_scale(self);
            em_hit_window_set(self, 1, 0x19, 0x10);
            return;
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 6.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 0.5f);
        }
        if (em_frame_check(self, 2, 30.0f, 0.0f) == 1) {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->offset_0x30C.vec_0x310.z < 0.0f) {
                self->offset_0x30C.vec_0x310.z = 0.0f;
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_ck(self, 2, 4, 0);
            return;
        }
        break;
    case 3:
        if (em_busy_ck(self) == 1) {
            if (em005_ally_near_ck(self, 0) == 1U) {
                em_state_set(self, 0xD, 3);
                return;
            }
            em_action_finish(self);
        }
        break;
    }
}

/* The demo tail spin: spins toward the side the target is on (the angle clamped to 25..155 degrees), turns back
 * through the wide turn table, backs off and finishes or hands over to the ally. */
void em005_act13_side_step(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    u32 angle;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (angle >= 0x8000) {
            self->state_0x006 = 1;
        } else {
            self->state_0x006 = 0;
        }
        switch (self->state_0x006) {
        case 0:
            em_mot_set(self, 0xD2, 4, 0);
            em_mot_speed_set(self, 1.2f);
            if (angle < 0x11C7) {
                angle = 0x11C7;
            } else if (angle > 0x6E39) {
                angle = 0x6E39;
            }
            break;
        case 1:
            em_mot_set(self, 0xD3, 4, 0);
            em_mot_speed_set(self, 1.2f);
            if (angle < 0x91C7) {
                angle = -0x6E39;
            } else if (angle > 0xEE39) {
                angle = -0x11C7;
            }
            break;
        }
        em_move_vec2_clr(self);
        {
            f32 scale = get_em_chg_scale(self);
            f32 rate = get_em_base_scale(self);
            self->offset_0x30C.vec_0x310.z = -15.625f * rate * scale;
        }
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0 + angle);
        if (angle > 0x8000) {
            angle = 0x10000 - angle;
        }
        self->timer_0x020 = (s16)angle;
        em_hit_window_set_default(self, 0, 0x18);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            em_turn_in_window(self, 0.0f, 64.0f, self->timer_0x020);
            break;
        case 1:
            em_turn_in_window(self, 0.0f, 64.0f, -self->timer_0x020);
            break;
        }
        if (em_frame_check(self, 3, 0.0f, 64.0f) == 1) {
            CancelFade(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_turn_seq_start(self, (void*)em005_turn_tbl_wide, 0, 1, 0);
        }
        break;
    case 2:
        if (em_turn_seq_step(self, (void*)em005_turn_tbl_wide) == 1) {
            self->state++;
            em_mot_set(self, 4, 4, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            self->state_0x006 = 0;
            em_mot_set(self, 6, 4, 0);
            em_approach_start(self, -500.0f, 0);
        }
        break;
    case 4:
        switch (self->state_0x006) {
        case 0:
            if (em_approach_step(self, 0, 0x40) != 1) {
                break;
            }
            self->state_0x006++;
            if (em_busy_ck(self) == 0) {
                em_mot_set(self, 1, 10, 0);
            }
        case 1:
            if (em_busy_ck(self) == 1) {
                if (em005_ally_near_ck(self, 1) == 1) {
                    em_state_set(self, 13, 4);
                } else {
                    em_action_finish(self);
                }
            }
            break;
        }
        break;
    }
}

/* 0x801D3C38 - the three-state "advance by 0x3E8 and close" action. */
void em005_act13_roar(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x14, 8, 0);
            em_stamina_add(self, 0x3E8);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act13_flip(struct _ENEMY_WORK* self, u32 mode) {
    nw4r::math::VEC3 sp34;
    struct EmRotWord3 rot;
    nw4r::math::VEC3 sp1C;
    nw4r::math::VEC3 sp10;
    u32 spC;
    u32 sp8;
    VEC3_ctor(&sp34);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0xE8, 6, 0);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = 15.0f * get_em_base_scale(self);
        self->vec_0x31C.z = 3.0f * get_em_base_scale(self);
        return;
    case 1:
        subVec3(&sp1C, &self->vec_0x36C, &self->pos);
        copyVec3(&sp34, &sp1C);
        calcVecAngXY(&sp34, &spC, &sp8);
        if (spC < 0x38EU) {
            spC = 0x38E;
        } else if (spC > 0x3C72U) {
            spC = 0x3C72;
        }
        em_turn_to_target(self, 0x200);
        copyVec3(&sp34, &self->offset_0x30C.vec_0x310);
        rotVecX(&sp34, spC);
        rotVecY(&sp34, self->field_0x1C0);
        em_hit_arm_set(self);
        self->field_0x1BC = (u32) em_angle_approach((u16) spC, (u16) self->field_0x1BC, 0);
        addVec3To(&self->pos, &sp34);
        self->offset_0x30C.vec_0x310.z += self->vec_0x31C.z;
        if (self->offset_0x30C.vec_0x310.z > (100.0f * get_em_base_scale(self))) {
            self->offset_0x30C.vec_0x310.z = 100.0f * get_em_base_scale(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0xE7, 0, 0x3A);
            em_hit_window_set(self, 0, 0x20, 0xB);
            return;
        }
        break;
    case 2:
        subVec3(&sp10, &self->vec_0x36C, &self->pos);
        copyVec3(&sp34, &sp10);
        calcVecAngXY(&sp34, &spC, &sp8);
        if (spC < 0x38EU) {
            spC = 0x38E;
        } else if (spC > 0x3C72U) {
            spC = 0x3C72;
        }
        em_turn_to_target(self, 0x200);
        copyVec3(&sp34, &self->offset_0x30C.vec_0x310);
        rotVecX(&sp34, spC);
        rotVecY(&sp34, self->field_0x1C0);
        em_hit_arm_set(self);
        self->field_0x1BC = (u32) em_angle_approach((u16) spC, (u16) self->field_0x1BC, 0x100);
        addVec3To(&self->pos, &sp34);
        self->offset_0x30C.vec_0x310.z += self->vec_0x31C.z;
        if (self->offset_0x30C.vec_0x310.z > (100.0f * get_em_base_scale(self))) {
            self->offset_0x30C.vec_0x310.z = 100.0f * get_em_base_scale(self);
        }
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            if ((s32) (u8) mode == 0) {
                em_mot_set(self, 0xDB, 0, 0);
                em_hit_window_set(self, 1, 0x21, 0x18);
            } else {
                em_mot_set(self, 0xF6, 0, 0);
                em_hit_window_set(self, 1, 0x23, 0x18);
            }
            self->timer_0x020 = (s32) (s16) self->field_0x1C0;
            self->field_0x1BC = 0;
            return;
        }
        break;
    case 3:
        if ((s32) (u8) mode == 0) {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_set(self, 0, 0x22, 0x10);
            }
            em_turn_in_window(self, 0.0f, 30.0f, 0x8000);
            if (em_frame_check(self, 3, 0.0f, 30.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_kick_rise_curve);
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_kick_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        } else {
            if (em_frame_check(self, 0, 20.0f, 0.0f) != 0) {
                em_hit_window_clear(self, 0);
            }
            em_turn_in_window(self, 0.0f, 62.0f, 0x10000);
            if (em_frame_check(self, 3, 0.0f, 62.0f) == 1) {
                self->offset_0x30C.vec_0x310.x = em_key_curve_eval(self, &em005_flip_end_rise_curve) / 3.0f;
                self->offset_0x30C.vec_0x310.z = em_key_curve_eval(self, &em005_flip_end_fwd_curve);
                rot.x = 0;
                rot.y = (s32) (u16) self->timer_0x020;
                rot.z = 0;
                em_move_offset_rot_apply(self, &rot);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_ck(self, 2, 4, 0);
            return;
        }
        break;
    case 4:
        if (em_busy_ck(self) == 1) {
            if (em005_ally_near_ck(self, 0) == 1U) {
                em_state_set(self, 0xD, 3);
                return;
            }
            em_action_finish(self);
        }
        break;
    }
}

/* The intro demo's key tracks: the `{frame, x, y, z}` move and fly tracks (`em_demo_key_apply`/`em_demo_key3_apply`)
 * and the `{frame, angle}` turn track. */
f32 em005_demo_move_keys[108] = {
    660.0f, 1585.16f, 1860.0f, -6553.1f, 662.0f, 1534.73f, 1833.41f, -6548.13f,
    664.0f, 1474.7f, 1792.47f, -6532.8f, 668.0f, 1335.88f, 1666.66f, -6468.65f,
    670.0f, 1262.13f, 1581.33f, -6418.62f, 672.0f, 1188.8f, 1480.74f, -6355.81f,
    674.0f, 1118.44f, 1364.67f, -6279.61f, 676.0f, 1053.55f, 1232.89f, -6189.42f,
    678.0f, 996.63f, 1085.16f, -6084.64f, 680.0f, 950.2f, 921.26f, -5964.65f,
    682.0f, 916.77f, 740.96f, -5859.95f, 684.0f, 898.85f, 544.03f, -5755.24f,
    686.0f, 886.8f, 0.0f, -5650.53f, 688.0f, 925.03f, 0.0f, -5552.09f,
    690.0f, 979.35f, 0.0f, -5459.91f, 692.0f, 1035.74f, 0.0f, -5367.73f,
    694.0f, 1093.59f, 0.0f, -5275.55f, 696.0f, 1152.3f, 0.0f, -5183.37f,
    700.0f, 1271.25f, 0.0f, -4999.01f, 703.0f, 1361.04f, 0.0f, -4860.74f,
    710.0f, 1570.72f, 0.0f, -4538.11f, 712.0f, 1629.97f, 0.0f, -4445.94f,
    714.0f, 1688.69f, 0.0f, -4353.76f, 716.0f, 1771.46f, 0.0f, -4257.42f,
    718.0f, 1804.04f, 0.0f, -4165.64f, 1019.0f, 1804.04f, 0.0f, -4165.64f,
    -1.0f, 1804.04f, 0.0f, -4165.64f,
};
f32 em005_demo_turn_keys[16] = {
    660.0f, 13.52f, 686.0f, 13.52f, 708.0f, 143.9f, 714.0f, 176.15f,
    716.0f, 187.65f, 719.0f, 191.3f, 1019.0f, 191.3f, -1.0f, 191.3f,
};
f32 em005_demo_fly_keys[68] = {
    1066.0f, 1263.35f, 331.58f, -4902.87f, 1130.0f, 1263.35f, 331.58f, -4902.87f,
    1134.0f, 1283.94f, 368.25f, -5307.05f, 1138.0f, 1297.8f, 404.93f, -5711.24f,
    1143.0f, 1310.12f, 446.87f, -5955.88f, 1149.0f, 1314.44f, 471.45f, -6249.45f,
    1150.0f, 1314.49f, 475.55f, -6274.14f, 1155.0f, 1314.74f, 464.28f, -6397.57f,
    1156.0f, 1314.02f, 462.03f, -6422.26f, 1158.0f, 1312.58f, 454.53f, -6471.63f,
    1161.0f, 1310.41f, 443.28f, -6521.3f, 1165.0f, 1302.0f, 428.28f, -6587.52f,
    1167.0f, 1297.8f, 416.05f, -6620.64f, 1170.0f, 1289.19f, 397.71f, -6670.3f,
    1173.0f, 1280.57f, 379.36f, -6697.9f, 1179.0f, 1263.35f, 334.22f, -6753.1f,
    -1.0f, 1263.35f, 334.22f, -6753.1f,
};

void em005_act13_demo(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    VEC3_ctor(&spot);
    switch (self->state) {
    case 0:
        self->state++;
        self->timer_0x020 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        break;
    case 1:
        if (em_demo_time_ck(0x10E) == 1) {
            self->state++;
            em_demo_enable(self);
            em_mot_set(self, 1, 0, 0);
            em_demo_rot_set(self, 32.3f, 40.3f, 21.6f);
            em_demo_pos_set(self, -2494.9f, 7556.9f, -13608.3f);
        }
        break;
    case 2:
        em_demo_rot_set(self, 32.3f, 40.3f, 21.6f);
        em_hit_arm_set(self);
        if (em_demo_time_ck(0x1B4) == 1) {
            self->state++;
            em_mot_set(self, 2, 0xA, 0);
        }
        break;
    case 3:
        em_demo_rot_set(self, 32.3f, 40.3f, 21.6f);
        em_hit_arm_set(self);
        if (em_frame_check(self, 1, 180.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 8, 8, 0);
            em_mot_speed_set(self, 1.7f);
        }
        break;
    case 4:
        em_demo_rot_set(self, 32.3f, 40.3f, 21.6f);
        em_hit_arm_set(self);
        if (em_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            setVector3(&spot, 0.0f, 0.0f, 200.0f);
            eft009_spawn_at_pos(self, 1, 5, 0, 0.7f, &spot);
        }
        if (em_demo_time_ck(0x294) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0xDA, 0, 0x56);
            em_mot_speed_set(self, 0.5f);
            em_demo_rot_set(self, 0.0f, 13.52f, 0.0f);
            em_demo_key_apply(self, em_demo_frame_get(), em005_demo_move_keys, em005_demo_turn_keys, 7, 2);
            em_height_set(self, self->pos.y - self->field_0x20C);
        }
        break;
    case 5:
        em_demo_key_apply(self, em_demo_frame_get(), em005_demo_move_keys, em005_demo_turn_keys, 7, 2);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (em_demo_time_ck(0x2AE) == 1) {
            self->state++;
            em_mot_set(self, 0xDB, 0, 0);
        }
        break;
    case 6:
        em_demo_key_apply(self, em_demo_frame_get(), em005_demo_move_keys, em005_demo_turn_keys, 7, 2);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &spot);
            spot.y = 5.0f + self->pos.y;
            eft_spawn_pos_in_area(&spot, self->area_no, 5, 0, 1.3f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 4, 0);
            em_mot_speed_set(self, 0.8f);
        }
        break;
    case 7:
        em_demo_key_apply(self, em_demo_frame_get(), em005_demo_move_keys, em005_demo_turn_keys, 7, 2);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 1, 72.0f, 0.0f) == 1) {
            self->state++;
            em_mot_speed_set(self, 1.0f);
        }
        break;
    case 8:
        em_demo_key_apply(self, em_demo_frame_get(), em005_demo_move_keys, em005_demo_turn_keys, 7, 2);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (em_demo_time_ck(0x3FC) == 1) {
            self->state++;
            em_mot_set(self, 2, 0, 0x110);
            em_demo_rot_set(self, 0.0f, 191.4f, 0.0f);
            em_demo_pos_set(self, 1804.0f, 0.0f, -4165.6f);
            em_height_set(self, 0.0f);
        }
        break;
    case 9:
        if (em_frame_check(self, 1, 286.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set(self, 0xDF, 0xA, 0);
        }
        break;
    case 10:
        if (em_demo_time_ck(0x42A) == 1) {
            em_demo_rot_set(self, 0.0f, -168.7f, 0.0f);
            em_demo_key3_apply(self, em_demo_frame_get(), em005_demo_fly_keys, 0);
            em_height_set(self, self->pos.y - self->field_0x20C);
        } else if (em_mot_frame_started_ck(self) == 0) {
            self->field_0x318 = em_key_curve_eval(self, em005_jump_curve);
            em_move_offset_rot_apply(self, &self->field_0x1BC);
        }
        if (em_demo_time_ck(0x454) == 1) {
            self->state++;
            em_mot_set(self, 0xDF, 0, 0);
            em_mot_speed_set(self, 0.35f);
        }
        break;
    case 11:
        em_demo_rot_set(self, 0.0f, -168.7f, 0.0f);
        em_demo_key3_apply(self, em_demo_frame_get(), em005_demo_fly_keys, 0);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (em_frame_check(self, 1, 17.0f, 0.0f) == 1) {
            self->state++;
            em_mot_speed_set(self, 1.0f);
        }
        break;
    case 12:
        em_demo_rot_set(self, 0.0f, -168.7f, 0.0f);
        em_demo_key3_apply(self, em_demo_frame_get(), em005_demo_fly_keys, 0);
        em_height_set(self, self->pos.y - self->field_0x20C);
        if (em_demo_time_ck(0x49C) == 1) {
            self->state++;
            em_demo_rot_set(self, 0.0f, 193.0f, 0.0f);
            em_demo_pos_set(self, 1985.8f, 0.0f, -6341.3f);
        }
        break;
    case 13:
        if (em_demo_time_ck(0x5AA) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0);
            em_mot_speed_set(self, 0.39f);
        }
        break;
    case 14:
        if (em_demo_time_ck(0x6E4) == 1) {
            self->state++;
            em_mot_set(self, 5, 0, 0);
            em_demo_rot_set(self, 0.0f, 193.0f, 0.0f);
            em_demo_pos_set(self, 1683.3f, 0.0f, -6181.6f);
            self->timer_0x020 = 0;
        }
        break;
    case 15:
        if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
            draw_shape_arm((u32)self, 0x1D, 0xA);
        }
        if (em_frame_check(self, 0, 124.0f, 0.0f) == 1) {
            setVector3(&spot, 0.0f, 0.0f, 200.0f);
            eft_em_spawn(self, 0, 0x1B, &spot, 1.0f);
        }
        if (em_frame_check(self, 3, 142.0f, 264.0f) == 1) {
            if ((self->timer_0x020 & 7) == 0) {
                setVector3(&spot, 0.0f, 0.0f, 200.0f);
                eft_em_spawn(self, 1, 0x1B, &spot, 1.0f);
            }
            self->timer_0x020++;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 2, 4, 0);
            em_mot_speed_set(self, 0.65f);
        }
        break;
    }
}

void em005_act13_demo_end(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_rot_set(self, 0.0f, 193.0f, 0.0f);
        em_demo_pos_set(self, 1683.3f, 0.0f, -6181.6f);
        em_height_set(self, 0.0f);
        em_demo_enable(self);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

void em005_act13_run(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em005_act13_fly_in(self);
        break;
    case 1:
        em005_act13_dive(self);
        break;
    case 2:
        em005_act13_lunge(self);
        break;
    case 3:
        em005_act13_side_step(self);
        break;
    case 4:
        em005_act13_roar(self);
        break;
    case 5:
        em005_act13_flip(self, 0);
        break;
    case 6:
        em005_act13_flip(self, 1);
        break;
    case 7:
        em005_act13_demo(self);
        break;
    case 8:
        em005_act13_demo_end(self);
        break;
    }
}

void em005_action_run(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em005_act0_run(self);
        break;
    case 1:
        em005_act1_run(self);
        break;
    case 2:
        em005_act2_run(self);
        break;
    case 3:
        em005_act3_run(self);
        break;
    case 4:
        em005_act4_run(self);
        break;
    case 7:
        em005_act7_run(self);
        break;
    case 10:
        em005_act10_se_play(self);
        break;
    case 11:
        em005_act11_se_play(self);
        break;
    case 12:
        em005_act12_run(self);
        break;
    case 13:
        em005_act13_run(self);
        break;
    }
}

/* In the alternate mode, while the head is tilted past the threshold, a body spark every 32 frames (the stronger one
 * past the second threshold); not during the two roar motions. */
void em005_body_spark(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 pos;
    u32 tilt;

    VEC3_ctor(&pos);
    if (em_alt_mode_ck(self) != 0) {
        tilt = (u16)calcVecAngX(&self->vec_0x76C);
        if ((u16)(tilt + 0x8000U) > 0x671B) {
            switch (em_get_mot_no(self)) {
            case 0xD8:
            case 0xE4:
                break;
            default:
                setVector3(&pos, 0.0f, -20.0f, 50.0f);
                if ((system_w.field_0x0c & 0x1F) == 0) {
                    if ((u16)(tilt + 0x8000) > 0x6E38) {
                        eft_spawn_type10(self, 0x16, 0x1C, &pos, 1.0f);
                    } else {
                        eft_spawn_type10(self, 0x15, 0x1C, &pos, 1.0f);
                    }
                }
                break;
            }
        }
    }
}

/* The footstep effect dispatcher: kind 0 places effect `id` at the body (or spawns it at a joint), kind 1 spawns the
 * area's ground effect and kind 2 the splash under the body or the joint. */
void em005_step_eft_set(struct _ENEMY_WORK* self, u8 kind, u8 id, u32 joint, s32 delta, f32 scale) {
    nw4r::math::VEC3 pos;

    VEC3_ctor(&pos);
    switch (kind) {
    case 0:
        switch (id) {
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
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
            if (joint == 0xFF) {
                pos.x = self->pos.x;
                pos.y = 5.0f + self->field_0x20C;
                pos.z = self->pos.z;
                eft009_set_pos(id, &pos, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
            } else {
                eft009_spawn_at_joint(self, joint, id, delta, scale);
            }
            break;
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
        eft_spawn_pos_in_area(&pos, self->area_no, id, self->field_0x1C0, scale);
        break;
    case 2:
        if (joint == 0xFF) {
            copyVec3(&pos, &self->pos);
        } else {
            get_joint_wpos_em(self, joint, &pos);
        }
        pos.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_type11(self, &pos, id, scale);
        break;
    }
}

void em005_motion_events(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    u32 area_flag;
    u8 flags;
    VEC3_ctor(&spot);
    em005_body_spark(self);
    switch (em_get_mot_no(self)) {
    case 0x8:
        if ((em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1)) {
            em005_step_eft_set(self, 0, 0, 0x25, 0, 1.2f);
            em005_step_eft_set(self, 0, 0, 0x2D, 0, 1.2f);
        }
        if ((em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 106.0f, 0.0f) == 1)) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
            em_camera_req(self, -1, 0);
        }
        if ((em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 114.0f, 0.0f) == 1)) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
        }
        break;
    case 0x9:
        if (em_after_frame_check(self, 0, 8.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.8f);
            em_camera_req(self, 8, 7);
        }
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 2.4f);
            em_camera_req(self, 0x11, 7);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x14, 8);
            em_hit_window_set(self, 1, 0x15, 0x10);
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 0, 0x11, 0, 1.2f);
        }
        break;
    case 0xD:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x16, 8);
            em_hit_window_set(self, 1, 0x17, 0x10);
        }
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 0, 8, 0, 1.2f);
        }
        break;
    case 0x1B:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 2, 0, 3, 0, 1.1f);
        }
        break;
    case 0x1E:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 2, 0, 3, 0, 1.1f);
        }
        break;
    case 0x25:
    case 0x26:
        if (em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 2, 0, 3, 0, 1.1f);
        }
        break;
    case 0x29:
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 5, 3, 0, 0.9f);
            setVector3(&spot, 0.0f, 0.0f, 100.0f);
            em_ground_impact_set_v(self, 1, 0, 0.9f, &spot);
        }
        break;
    case 0x2A:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 5, 3, 0, 0.9f);
        }
        break;
    case 0x41:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 3, 0, 0.8f);
            setVector3(&spot, 0.0f, 0.0f, 100.0f);
            em_ground_impact_set_v(self, 2, 0, 1.7f, &spot);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x44:
        if (em_after_frame_check(self, 0, 12.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 2, 0, 3, 0, 1.1f);
        }
        break;
    case 0x64:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
        }
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
        }
        break;
    case 0x65:
        if (em_after_frame_check(self, 0, 68.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
        }
        if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 0, 4, 0x2AAB, 1.5f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 0, 4, 0xD556, 1.5f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 5, 4, 0, 1.0f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x72:
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 1, 0x19, 0, 0.6f);
        }
        break;
    case 0x74:
        if (em_after_frame_check(self, 0, 254.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 1, 0x19, 0, 0.6f);
        }
        break;
    case 0x78:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 0, 0x25, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 0, 0x2D, 0, 1.2f);
        }
        if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 0x18, 0, 0.8f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x7D:
        if (em_after_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.8f);
        }
        if (em_after_frame_check(self, 0, 520.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 1, 4, 0, 1.0f);
        }
        break;
    case 0x7F:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 2, 4, 0, 1.2f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x80:
        if (em_after_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x1C, 0, 1.8f);
        }
        break;
    case 0x82:
        if (em_after_frame_check(self, 0, 138.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 1, 0x1A, 0, 0.7f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x83:
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
            em_camera_req(self, -1, 7);
        }
        if ((em_after_frame_check(self, 0, 72.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 84.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1)) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
        }
        break;
    case 0x84:
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
        }
        break;
    case 0xC9:
    case 0xCA:
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 6, 4, 0, 2.0f);
        }
        break;
    case 0xCB:
    case 0xCC:
        if ((em_after_frame_check(self, 0, 10.0f, 0.0f) == 1) || (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1)) {
            em005_step_eft_set(self, 2, 0, 4, 0, 1.1f);
        }
        break;
    case 0xCD:
    case 0xCF:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 5, 4, 0, 0.9f);
        }
        break;
    case 0xCE:
    case 0xD0:
        if (em_after_frame_check(self, 0, 48.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 2, 0, 4, 0, 1.1f);
        }
        break;
    case 0xD1:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.7f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xD2:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            setVector3(&spot, 0.0f, 0.0f, 200.0f);
            eft009_spawn_at_pos(self, 8, 2, 0, 1.6f, &spot);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
        }
        break;
    case 0xD3:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            setVector3(&spot, 0.0f, 0.0f, 200.0f);
            eft009_spawn_at_pos(self, 0x11, 2, 0, 1.6f, &spot);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
        }
        break;
    case 0xD4:
        if ((u8) self->action == 0xA) {
            if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
                em_hit_window_set(self, 0, 3, 8);
                em_hit_window_set(self, 1, 4, 0x18);
            }
            if (em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em_hit_window_clear(self, 1);
                em_hit_window_set(self, 1, 0x10, 0x10);
            }
        }
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
            em_camera_req(self, 8, 7);
        }
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
            em_camera_req(self, 0x11, 7);
        }
        break;
    case 0xD5:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x2D, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xD7:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.9f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xD9:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x25, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 2.0f);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xDB:
        if (em_after_frame_check(self, 0, 4.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.9f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xDC:
        if (((s32) (self->flags_0x836 & 1) == 0) && (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1)) {
            setVector3(&spot, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 1, 0x1A, &spot, 1.0f, 0xC);
            eft_em_spawn_joint(self, 1, 0x1C, &spot, 1.0f, 0xC);
        }
        break;
    case 0xDD:
        if (((s32) (self->flags_0x836 & 1) == 0) && (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1)) {
            setVector3(&spot, 0.0f, 0.0f, 0.0f);
            eft_em_spawn_joint(self, 1, 0x19, &spot, 1.0f, 0xC);
            eft_em_spawn_joint(self, 1, 0x1B, &spot, 1.0f, 0xC);
        }
        break;
    case 0xDE:
    case 0xE1:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.9f);
        }
        break;
    case 0xDF:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            setVector3(&spot, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x7C, 0x14, &spot, 1.0f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xE2:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            setVector3(&spot, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, 0x7D, 0xB, &spot, 1.0f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xE4:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 0x11, 0, 1.6f);
        }
        if (em_after_frame_check(self, 0, 32.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 2, 8, 0, 1.6f);
        }
        break;
    case 0xE9:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 2, 0, 3, 0, 1.1f);
        }
        break;
    case 0xEC:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 0, 5, 3, 0, 0.9f);
        }
        break;
    case 0xED:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_ground_impact_set_v(self, 1, 0, 1.0f, NULL);
        }
        break;
    case 0xEF:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.7f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0xF0:
    case 0xF1:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.7f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xF2:
    case 0xF3:
        if (em_after_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.7f);
            em_camera_req(self, -1, 7);
        }
        break;
    case 0xF6:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em005_step_eft_set(self, 1, 4, 4, 0, 0.7f);
            em_camera_req(self, -1, 1);
        }
        break;
    }
    area_flag = 0;
    if ((s32)stage_map_kind_get(self->field_0x1E0) == 4 && ((s32)self->area_no == 4 || (s32)self->area_no == 6)) {
        area_flag = 1;
    }
    if (em_alt_mode_ck(self) == 1 || area_flag == 1) {
        self->field_0x761 = (u8)(self->field_0x761 | 1);
    } else {
        flags = self->field_0x761;
        if ((flags & 1) != 0) {
            self->field_0x761 = (u8)(flags & 0xFE);
        }
    }
    if (area_flag == 1 && self->field_0x762 == 0) {
        self->field_0x761 = (u8)(self->field_0x761 | 2);
        return;
    }
    flags = self->field_0x761;
    if ((flags & 2) != 0) {
        self->field_0x761 = (u8)(flags & 0xFD);
    }
}

void em005_tev_color_update(struct _ENEMY_WORK* self) {
    _GXColor color;
    ((MHchar*)self->char_0x024)->getTevKColor(5, GX_KCOLOR3, &color);
    if (em_alt_mode_ck(self) == 1) {
        if (color.a < 251) {
            color.a += 4;
        } else {
            color.a = 255;
        }
    } else {
        if (color.a > 4) {
            color.a -= 4;
        } else {
            color.a = 0;
        }
    }
    ((MHchar*)self->char_0x024)->setTevKColor(5, GX_KCOLOR3, &color);
}

u32 em005_ally_near_ck(struct _ENEMY_WORK* self, u8 arg1) {
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    nw4r::math::VEC3 d;
    nw4r::math::VEC3 c;
    struct _ENEMY_WORK* other;
    f32 dist;
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    other = em_ally_find(self, 0x1C, 0);
    if (other != NULL && em_act11_wait_ck(other) == 1) {
        if (arg1 == 0) {
            return 1;
        }
        copyVec3(&b, setVec3(&d, 0.0f, 0.0f,
        600.0f * get_em_chg_scale(self)));
        rotVecY(&b, self->field_0x1C0);
        addVec3(&c, &self->pos, &b);
        copyVec3(&a, &c);
        dist = calcDistanceSqXZ(&a, &other->pos);
        if (dist < (400.0f * get_em_chg_scale(self)) * (400.0f * get_em_chg_scale(self))) {
            return 1;
        }
    }
    return 0;
}

u32 em005_act13_early_ck(struct _ENEMY_WORK* self) {
    if (self->action == 13 && self->state_sub <= 4) {
        return 1;
    }
    return 0;
}

u32 em005_capsule_point_ck(u8 index, nw4r::math::VEC3* point) {
    nw4r::math::CAPSULE capsule;
    PlCapsuleBox box;
    eft_capsule_ctor(&capsule);
    eft_shape_init((struct EftShape*)&box);
    if (index >= 3) {
        return 0;
    }
    Em005Capsule* entry = &em005_capsule_tbl[index];
    capsule_box_from_points_v(&entry->start.pos, &entry->end.pos, entry->radius, &capsule);
    pl_capsule_box_set((const PlCapsule*)&capsule, &box);
    return pl_capsule_point_ck(point, &box) == 1;
}

u32 em005_holder_in_area_ck(struct _ENEMY_WORK* self) {
    if (self->field_0x382 != 0xFF && (s32)self->field_0x380 == 1) {
        _PLW* holder = (_PLW*)em_move_work_pick(self->state_0x381, self->field_0x382);
        if (holder->slot_active != 0 && holder->area_0x16 == self->area_no && (u8)(holder->water_area_0x5A6 & 0x7F) == 1) {
            return 1;
        }
    }
    return 0;
}

/* Of the `count` area data points from `first` on, returns the index of the one nearest `target` (XZ distance);
 * the parameter holds the answer once a point is measured, so `count` comes back when there is none. */
u8 em005_route_point_pick(struct _ENEMY_WORK* self, u32 first, u8 count, nw4r::math::VEC3* target) {
    nw4r::math::VEC3 pos;
    f32 nearest;
    u8 id;
    s32 i;
    u8 num;

    VEC3_ctor(&pos);
    i = 0;
    id = first;
    num = count;
    for (; i < num; i++) {
        em_data_point_get(self, (u16)((self->area_no & 0xF) << 8 | id), &pos);
        if (i == 0) {
            nearest = calcDistanceSqXZ(target, &pos);
            count = i;
        } else {
            f32 dist = calcDistanceSqXZ(target, &pos);
            if (nearest > dist) {
                nearest = dist;
                count = i;
            }
        }
        id++;
    }
    return count;
}

u32 em005_state_query(struct _ENEMY_WORK* self, u32 arg1) {
    nw4r::math::VEC3 pos;
    u32 t;
    u8 sel;
    VEC3_ctor(&pos);
    sel = (u8)arg1;
    switch (sel) {
    case 0:
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 4) {
            switch (self->area_no) {
            case 2:
                setVector3(&pos, 900.0f, 20.0f, -1350.0f);
                if (calcDistanceSqXZ(&self->pos, &pos) <= 1000000.0f) {
                    return 0;
                }
                break;
            case 3:
                setVector3(&pos, 2750.0f, 7.0f, -8700.0f);
                if (calcDistanceSqXZ(&self->pos, &pos) <= 1000000.0f) {
                    return 0;
                }
                break;
            case 7:
                setVector3(&pos, -1650.0f, 0.0f, -1100.0f);
                if (calcDistanceSqXZ(&self->pos, &pos) <= 1000000.0f) {
                    return 0;
                }
                setVector3(&pos, 2100.0f, 567.0f, 3740.0f);
                if (calcDistanceSqXZ(&self->pos, &pos) <= 1000000.0f) {
                    return 1;
                }
                break;
            }
        }
        return 0xFF;
    case 1:
        return em_ally_find(self, 0x1C, 1) != NULL;
    case 2:
        return self->action_0x328.armed_0x328.field_0x330;
    case 3:
        if (em_parts_damage_level_get(self, 2) >= 1) {
            return 1;
        }
        break;
    case 4:
        if (em_parts_damage_level_get(self, 3) >= 1) {
            return 1;
        }
        break;
    case 5:
        switch (self->field_0x010) {
        case 2:
            sel = em005_route_point_pick(self, 0x11, 3, &self->pos);
            break;
        default:
            sel = em005_route_point_pick(self, 0xC, 3, &self->pos);
            break;
        }
        switch (sel) {
        case 0:
            if (em005_capsule_point_ck(0, &self->vec_0x36C) == 1 ||
                em005_capsule_point_ck(1, &self->vec_0x36C) == 1 ||
                em005_capsule_point_ck(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        case 1:
            if (em005_capsule_point_ck(0, &self->vec_0x36C) == 1 ||
                em005_capsule_point_ck(1, &self->vec_0x36C) == 1 ||
                em005_capsule_point_ck(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        case 2:
            if (em005_capsule_point_ck(0, &self->vec_0x36C) == 1 ||
                em005_capsule_point_ck(1, &self->vec_0x36C) == 1 ||
                em005_capsule_point_ck(2, &self->vec_0x36C) == 1) {
                return 0;
            }
            break;
        }
        if (em005_holder_in_area_ck(self) == 1) {
            return 0;
        }
        switch (self->field_0x010) {
        case 2:
            return (u8)(em005_route_point_pick(self, 9, 3, &self->vec_0x36C) + 1);
        }
        return (u8)(em005_route_point_pick(self, 6, 3, &self->vec_0x36C) + 1);
    case 6:
        t = em005_holder_in_area_ck(self);
        return (u32)((1 - t) | (t - 1)) >> 31;
    }
    return 0;
}

void em005_death_action_get(struct _ENEMY_WORK* self, u8* out1, u8* out2) {
    *out1 = 12;
    *out2 = 0;
    self->pos.y += 5000.0f;
}

s32 em005_part_hit_ck(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0 && em_alt_mode_ck(self) == 1 && self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

s32 em005_part_ck(struct _ENEMY_WORK* self, u8 arg1) {
    if (arg1 == 0 && em_eft_active_ck(self) == 1 && self->field_0x1E2 == 0) {
        return 1;
    }
    return 0;
}

void em005_part_arm(struct _ENEMY_WORK* self) {
    if (self->action_0x328.armed_0x328.field_0x330 == 0xFF) {
        if ((s32)stage_map_kind_get(self->field_0x1E0) == 4) {
            if (self->area_no == 5 && self->field_0x9F6 == 7) {
                self->action_0x328.armed_0x328.field_0x330 = 1;
            } else {
                self->action_0x328.armed_0x328.field_0x330 = 0;
            }
        } else {
            self->action_0x328.armed_0x328.field_0x330 = 0xFF;
        }
    }
}

/* The area-change check on map kind 4: the area break, the follow pick once the 900-frame gate is passed, the roam
 * pick or the escape test. */
s32 em005_area_change_ck(struct _ENEMY_WORK* self, u16 timer) {
    u8 map = stage_map_kind_get(self->field_0x1E0);
    s32 moved;
    u32 area;

    if ((s32)map != 4) {
        return 0;
    }
    moved = 0;
    area = 0xFF;
    if ((s32)map == 4) {
        area = 7;
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
    if (moved == 0 && self->value_0x452 >= 900) {
        if (em_pack_area_pick_v(self, 0x1C, (s32)map == 4 ? (u8)5 : (u8)0xFF, timer, (u8*)em005_follow_area_tbl,
                                (u8*)em005_follow_area_tbl_near) == 1) {
            return 1;
        }
    }
    if (em_area_roam_pick(self, timer) == 1) {
        return 1;
    }
    return em_area_leave_ck_v(self) == 1;
}

void em005_spawn_rec_fill(EmSpawnRec* rec, u8 a1, u16 a2, u16 a3) {
    nw4r::math::VEC3 pos;
    setVec3(&pos, 0.0f, -20.0f, 50.0f);
    rec->id = 29;
    copyVec3(&rec->pos, &pos);
    rec->field_0x10 = a1;
    rec->field_0x12 = a2;
    rec->field_0x14 = a3;
}

/* The user-data helper's destructor: the base record's, then the storage when `flag` is positive. */
Em005UserData* em005_user_data_dtor(Em005UserData* self, s16 flag) {
    if (self != NULL) {
        em_res_user_data_dtor_v(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}


/* The unit's `.bss`: the four two-vector records MWCC's `__sinit` seeds. */
Em005VecPair em005_vec_pair_0(10.0f, -0.4f);
Em005VecPair em005_vec_pair_1(10.0f, -0.4f);
Em005VecPair em005_vec_pair_2(70.0f, -0.6f);
Em005VecPair em005_vec_pair_3(10.0f, -0.4f);

/* The three capsules `em005_capsule_point_ck` tests a point against. */
Em005Capsule em005_capsule_tbl[3] = {
    {Em005CapsulePoint(972.4f, 0.0f, -1978.9f), Em005CapsulePoint(-284.9f, -5.0f, -3844.0f), 1400.0f},
    {Em005CapsulePoint(-2853.6f, 0.0f, 21.2f), Em005CapsulePoint(267.0f, 0.0f, -2288.1f), 1400.0f},
    {Em005CapsulePoint(-3415.3f, 0.0f, 3388.4f), Em005CapsulePoint(-2674.6f, 0.0f, 94.4f), 1400.0f},
};
