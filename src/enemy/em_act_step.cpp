/* enemy/em_act_step.cpp - enemy 026's program (`em026_prog_tbl`): the init, frame update, action run, motion events,
 *   state query, death action and area-keep entry points, the per-action step machines and their dispatchers, the
 *   sound-effect players and their tables, the demo key tables, the user-data accessor class `EmActStepUserData`
 *   (`enemy/em_act_step.h`) and the static initializer.
 * RANGE. .text 0x8032C920-0x80330194 (70 functions, in address order); .ctors 0x8056F3A0-0x8056F3A4
 *   (`__sinit_\em_act_step_cpp`), .data 0x805DFC30-0x805E0510 (`em026_prog_tbl` first; the jump tables, the sound
 *   programs, the demo keys and the class table `__vt__17EmActStepUserData` last), .bss 0x806BE328-0x806BE340
 *   (`em026_vec_pair`), .sdata 0x80792D18-0x80792D28, .sdata2 0x8079B108-0x8079B210, extab, extabindex.  The band's
 *   last four functions are `enemy/em_act_step_tail.cpp`.
 * SEAM. The band's end 0x8033041C (against `hud/pl_frame_sync.cpp`) is settled: extabindex entry 57 (0x800357A8)
 *   is `eft_frame_state_init`; the `.sdata2` run cuts at 0x8079B20C | 0x8079B210 with no label shared and MWCC's u32->f32
 *   magic emitted once per side (0x8079B140 and 0x8079B228); the `.data` run cuts at 0x805E0510.  The cut at
 *   0x80330194 is the reconciled candidate's and unproven.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off`, `#pragma fp_contract off` and `#pragma pool_data off`.
 * NAMES. The unit is enemy 026's program: `em026_prog_tbl` (0x805DFC30) opens its `.data` and holds this range's entry
 *   points (the file name predates that finding).  GUESS: every function, table and field name, from its body, its
 *   program-table slot (`Em022ProgTbl`'s scheme) or the action it serves: `em_act_step_<n>` is action n's dispatcher
 *   (`em026_action_run` switches on `action`), `em_act_arm_<motion>` one phase that arms a motion, `_hit<a>_<b>` the
 *   pair handed to `em_state_set`; the action 7 machines (`em026_act7_*`) leap at, cling to (`grip_0x328`, the
 *   player's joint matrix) and are shaken off a player; the sound programs are named after the first `state_sub`
 *   that plays them (`em026_se_tbl_<sub>`, `em026_se_alt_tbl_<sub>`), their records `<table>_rec<entry>`.  The
 *   player helpers `pl_cling_req_set`/`pl_shake_off_ck`/`pl_guard_latch_ck` are registry names.
 * RESIDUALS. All 70 rows written; 3 short of 100.
 *  - `em_act_face_away`: self/player/grip pointer take r30/r31/r29 where retail has r29/r30/r31, and the three VEC3
 *    temporaries sit one slot apart (a declaration-order search over 400 orders and a grip-pointer-free spelling
 *    measured no better);
 *  - `em026_act3_lurk`: retail coalesces the companion pointer with the player pointer (r26) and the torch flag with
 *    the loop index (r28); ours keeps them apart (declaration orders and a direct `ainpc_w` spelling tried);
 *  - `em_act_arm_mot203_204`: `(action == 1) + 0xCB` lands in r0 where retail computes it in r4 (operand swap, a
 *    `u16` parameter view and a separate motion local tried; `enemy/em016_prog.cpp` records the same shape).
 *  - `em026_area_keep_set`'s source record is this unit's `Em026KeepSrc`, the same three words as
 *    `enemy/em040_ai.cpp`'s local `EmAreaKeepSrc` (one shared type once that unit's copy moves to a header).
 *  - flip: the three `Pl/pl_act.cpp` callees are registry names (`pl_cling_req_set`, `pl_shake_off_ck`,
 *    `pl_guard_latch_ck`) that are not map rows yet.
 * SHAPES. `#pragma peephole off`, `#pragma fp_contract off` and `#pragma pool_data off` (each table is addressed with
 *   its own `lis`/`addi`) over the bodies.  `EmActStepUserData` derives from `ResUserDataAc`, so MWCC emits its table;
 *   it is allocated with `operator new` and built with placement `new`.  The static initializer is MWCC's own `__sinit`
 *   for `em026_vec_pair` (a struct with an inline constructor).  Float arguments retail evaluates before the integers
 *   go through float-first views (`em_mot_speed_set_ff`, `em_gauge_period_step_v`); `em_seat_aim_ck` returns `s8`
 *   (its callers `extsb` at each compare); the death action's cases keep retail's 0/2/3/1/6 order.
 */

#include "types.h"
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "ef.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/em_act_step.h"
#include "enemy/em_pop.h" /* quest_move_sub_state_4_get (the owner's header, rule 2) */
#include "unsplit/enemy.h"
#include "pl.h"
#include "Pl/fn_802693C4.h"
#include "Pl/fn_8028F66C.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_cling_req_set.h"
#include "Pl/pl_shake_off_ck.h"
#include "Pl/pl_guard_latch_ck.h"
#include "enemy/em_flag_0x8AF_set.h"
#include "enemy/em_flag_0x8AF_ck.h"
#include "enemy/em_flag_0x8AE_ck.h"
#include "enemy/fn_80138074.h"   /* em_res_user_data_ck / em_res_user_data_set */
#include "MSL/new.h"            /* the placement `operator new` */
#include "g3d/g3d_resmat.h"     /* nw4r::g3d::ResMdl (rule 2) */
#include "enemy/em_act_arm_unless_down.h"
#include "enemy/fn_80147CE0.h"  /* EmSpawnRec, em_spawn_rec_init */
#include "enemy/fn_8012EC74.h"  /* em_get_mot_no, em_after_frame_check */
#include "enemy/fn_8012BDF4.h"  /* em_act_ck */
#include "enemy/fn_8011D448.h"  /* em_gauge_period_step */
#include "enemy/enemy_control.h" /* the em_demo_* helpers */
#include "enemy/em_se_record.h" /* EmSeEntry / EmSeRecord / EmSeOffset */
#include "enemy/em_se_tbl_play.h"
#include "enemy/em_se_tbl_play_alt.h"
#include "enemy/em_hit_window_set.h"
#include "enemy/em_hit_window_clear.h"
#include "enemy/em_reaction_req.h"
#include "enemy/em_sight_player_ck2.h"
#include "enemy/em_ground_rec_clear.h"
#include "enemy/CancelFade.h"
#include "enemy/joint_mtx_store.h"
#include "ef/eft007.h"          /* eft007_part_spawn */
#include "ef/eft009.h"          /* eft009_spawn_at_joint */
#include "lobby/fn_8030121C.h"  /* eft_em_spawn */
#include "stage/shell_set_func_ptr.h" /* shell_set_func_ptr and its slots */
#include "stage/stg_w.h"        /* get_now_areano */
#include "sound/fn_800D7F54.h"  /* se_req_pos_ps */
#include "ai/ainpc_w.h"
#include "ai/ainpc.h"
#include "ai/ai_torch_ck.h"

#ifdef __cplusplus

extern "C" {
#endif

/* The out-of-range callees whose owner's header does not declare them; each spelling is the callee's own body. */
void vec3_random_offset(void* out, const void* in, f32 scale, s32 a);
void em_action_finish_fall(struct _ENEMY_WORK* self);
f32 calcVecDistXZ(const void* a, const void* b);
void em_attack_done_set(struct _ENEMY_WORK* self);
void em_ground_snap(struct _ENEMY_WORK* self);
void eft_rot_vec_copy(_CP_VECTOR* dst, const _CP_VECTOR* src);
u32 quest_sub_state_end_ck(u32 a);
void em_move_vec_set(struct _ENEMY_WORK* self, void* vec);
void em_attack_start(struct _ENEMY_WORK* self);


#ifdef __cplusplus
}
#endif

/* The enemy work API this band calls, at C++ scope so the front-end reproduces the map's mangled name
 * (rule 9); each spelling is the callee's own body. */
u32 get_move_work_max(u8 kind);
void* get_move_work_adrs(u8 kind);
s32 em_die_ck(struct _ENEMY_WORK* self);

/* `em_fall_start` through the signature its body has: it reads a float in f1 (the shared declaration takes none). */
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
/* `em_mot_speed_set` with its float argument listed first, so it is evaluated before `self` (retail order). */
#define em_mot_speed_set_ff ((void (*)(f32, struct _ENEMY_WORK*))em_mot_speed_set)
/* `em_gauge_period_step` and `em_turn_to_target` with their arguments in the order and widths retail evaluates them
 * (the float before the integers; a `u16` turn rate). */
#define em_gauge_period_step_v ((void (*)(struct _ENEMY_WORK*, f32, s32, s16))em_gauge_period_step)
#define em_turn_to_target_c1 ((u32 (*)(struct _ENEMY_WORK*, u16))em_turn_to_target)

/* The +0x3C `ShellSetFuncs` slot spelled without its trailing table argument (the table load is then emitted
 * last, in r6, as retail has it). */
typedef void (*Em026ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

/* The spawn-placement source the area-keep slot reads (only its +0x04 rotation word).
 * size: 0x0C (the extent `enemy/em040_ai.cpp`'s copy of the record reaches) */
struct Em026KeepSrc {
    /* +0x00 */ u32 leap_0x00;
    /* +0x04 */ u32 rot_0x04;
    /* +0x08 */ u32 angle_0x08;
};

/* A point and its per-frame step, built by MWCC's `__sinit`: each vector is set through `setVec3` into a temporary and
 * copied with `assignVec3`.  size: 0x18 */
struct Em026VecPair {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ nw4r::math::VEC3 step;

    /* Builds (0, y, z) and (0, dy, dz). */
    Em026VecPair(f32 y, f32 z, f32 dy, f32 dz)
    {
        nw4r::math::VEC3 b;
        nw4r::math::VEC3 a;

        assignVec3((Vec*)&pos, (Vec*)setVec3(&a, 0.0f, y, z));
        assignVec3((Vec*)&step, (Vec*)setVec3(&b, 0.0f, dy, dz));
    }
};

extern "C" {

/* The program-table entry points, defined below in address order. */
void em026_init(_ENEMY_WORK* self, u8 mode);
void em_act_die_step(_ENEMY_WORK* self);
void em026_action_run(_ENEMY_WORK* self);
void em_work_dtor_del(void);
void em_work_noop(void);
void em026_motion_events(_ENEMY_WORK* self);
void em026_tev_color_update(void);
u8 em026_state_query(_ENEMY_WORK* self, u8 kind);
u32 em026_cry_se_play(_ENEMY_WORK* self);
void em026_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub);
void em026_rearm_noop(void);
u32 em026_state_clear_ck(_ENEMY_WORK* self);
void em026_area_keep_set(_ENEMY_MINI_WORK* mini, nw4r::math::VEC3* pos, Em026KeepSrc* src, u8 flags);
void em026_act3_lurk(_ENEMY_WORK* self);

/* The em026 program table (`em026_prog_tbl`): the entry points the shared enemy program driver calls.  Slot names
 * are a GUESS from the functions they hold (`Em022ProgTbl`'s scheme).
 * size: 0x6C */
struct Em026ProgTbl {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(void);
    /* +0x14 */ void (*event_notify_0x14)(void);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(void);
    /* +0x24 */ u8 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 (*cry_se_0x28)(_ENEMY_WORK* self);
    /* +0x2C */ u32 pad_0x2C;
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*rearm_0x34)(void);
    /* +0x38 */ u32 pad_0x38[5];
    /* +0x4C */ u32 (*state_clear_ck_0x4C)(_ENEMY_WORK* self);
    /* +0x50 */ u32 pad_0x50[6];
    /* +0x68 */ void (*area_keep_set_0x68)(_ENEMY_MINI_WORK* mini, nw4r::math::VEC3* pos, Em026KeepSrc* src, u8 flags);
};

Em026ProgTbl em026_prog_tbl = {
    0,
    em026_init,
    em_act_die_step,
    em026_action_run,
    em_work_dtor_del,
    em_work_noop,
    em026_motion_events,
    0,
    em026_tev_color_update,
    em026_state_query,
    em026_cry_se_play,
    0,
    em026_death_action_get,
    em026_rearm_noop,
    {0, 0, 0, 0, 0},
    em026_state_clear_ck,
    {0, 0, 0, 0, 0, 0},
    em026_area_keep_set,
};

/* The `.bss` vector pair three sound records point at (MWCC's `__sinit` seeds it; defined at the end). */
extern Em026VecPair em026_vec_pair;

} /* extern "C" */

#pragma peephole off
#pragma fp_contract off
#pragma pool_data off

/* Rotates the effect's local offset vector by the work record's own y angle, then hands it to the
 * effect placer. */
extern "C" void em_eff_offset_set(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    offset.x = 0.0f;
    offset.y = 0.0f;
    offset.z = -35.0f;
    rotVecY(&offset, self->field_0x1C0);
    nw4r::math::VEC3 out = offset;
    em_move_vec_set(self, &out);
}

/* Clears the effect offset block and the state word next to it. */
extern "C" void em_eff_offset_clr(_ENEMY_WORK* self)
{
    EmGripBlock* block = &self->grip_0x328;

    setVector3(&block->offset, 0.0f, 0.0f, 0.0f);
    block->joint_0x0C = 0;
}

/* Faces the work record away from the player: with the player in play and fewer than three of its act bits set,
 * turns the record's y angle to the reverse bearing and stores the sub-step. */
extern "C" u32 em_act_face_away(_ENEMY_WORK* self, _PLW* pl)
{
    nw4r::math::VEC3* offset = &self->grip_0x328.offset;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 seed;
    nw4r::math::VEC3 target;
    u32 ang_a;
    u32 ang_b;
    u32 bits;
    s32 i;
    s32 count;

    if (pl == NULL) {
        return 0;
    }
    if (pl->slot_active == 0) {
        return 0;
    }
    if (Pl_master_ck(pl) != 1) {
        return 0;
    }
    if (quest_sub_state_end_ck(1) != 0) {
        return 0;
    }
    if (quest_move_sub_state_4_get() != 0) {
        return 0;
    }

    bits = pl->field_0x3B0;
    count = 0;
    for (i = 0; i < 32; i++) {
        if ((bits & (1u << i)) != 0) {
            count++;
        }
    }
    if (count >= 3) {
        return 0;
    }

    VEC3_ctor(&dir);
    setVec3(&seed, 0.0f, 0.0f, 10.0f);
    copyVec3(offset, &seed);

    subVec3(&target, &self->pos, &pl->vec_0x03C);
    copyVec3(&dir, &target);
    calcVecAngXY(&dir, &ang_a, &ang_b);
    rotVecY(offset, (u16)(ang_a - pl->field_0x058));

    subVec3(&target, &pl->vec_0x03C, &self->pos);
    copyVec3(&dir, &target);
    calcVecAngXY(&dir, &ang_a, &ang_b);
    self->field_0x1C0 = ang_a - pl->field_0x058;
    self->grip_0x328.joint_0x0C = 3;
    pl_cling_req_set(pl, self->group);
    return 1;
}

/* Places the effect at the work record's position when the ground record for its +0x1A id is found,
 * and copies that record's rotation into the record. */
extern "C" void em_eff_ground_set(_ENEMY_WORK* self)
{
    EmGroundRec rec;

    em_ground_rec_clear(&rec);
    if (em_ground_rec_find(self->field_0x01A, &rec) != 0) {
        vec3_random_offset(&self->pos, &rec.pos_0x08, 500.0f, 0);
        eft_rot_vec_copy((_CP_VECTOR*)&self->field_0x1BC, (_CP_VECTOR*)&rec.field_0x14);
    }
}

/* The program table's init: restores the grip latch kept across a store, installs the accessor once, starts the
 * entry action the area kind asks for and, on a mode-2 entry with a saved position, returns to it. */
extern "C" void em026_init(_ENEMY_WORK* self, u8 mode)
{
    EmActStepUserData* record;
    EmRunPos* run = &self->run_0xB04;

    if (run->flags == 1) {
        self->grip_0x328.clung_0x10 = 1;
    } else {
        self->grip_0x328.clung_0x10 = 0;
    }
    self->grip_0x328.cling_frames_0x12 = 0;
    self->field_0x835 = 1;
    self->hp_ratio_0x7A8 = 0.25f;
    if (em_res_user_data_ck(self) == 0) {
        record = (EmActStepUserData*)operator new(sizeof(EmActStepUserData));
        new (record) EmActStepUserData;
        em_res_user_data_set(self, record);
    }
    switch (self->field_0x00A) {
    case 1:
        em_move_mode_set(self, 0);
        em_act_arm_unless_down(self, 1, 6);
        em_state_refresh(self);
        break;
    case 2:
        em_fall_height_get(self);
        em_fall_start(self);
        em_act_arm_unless_down(self, 3, 1);
        break;
    case 3:
        em_fall_height_get(self);
        em_fall_start(self);
        em_act_arm_unless_down(self, 3, 10);
        em_state_refresh(self);
        break;
    case 4:
        em_move_mode_set(self, 0);
        em_act_arm_unless_down(self, 1, 2);
        em_state_refresh(self);
        break;
    case 5:
        em_fall_height_get(self);
        em_fall_start(self);
        em_act_arm_unless_down(self, 3, 3);
        em_state_refresh(self);
        break;
    case 6:
        em_fall_height_get(self);
        em_fall_start(self);
        em_act_arm_unless_down(self, 3, 20);
        em_state_refresh(self);
        break;
    }
    if ((s32)mode == 2 && (run->valid_0x0F & 1) != 0) {
        copyVec3(&self->pos, &run->pos);
        self->field_0x1C0 = run->angle;
    }
    em_eff_offset_clr(self);
}

/* 0x8032CEA8 (0x3C): Constructs the accessor: the base first, then this class's table. */
EmActStepUserData::EmActStepUserData()
{
}

/* An empty body the target keeps after the accessor's constructor. */
extern "C" void em_work_dtor_del(void)
{
}

/* The range's other empty stub - a virtual override whose body the target does not emit either. */
extern "C" void em_work_noop(void)
{
}

/* Steps the death action: while `em_die_ck` refuses it and the +0x834 byte is set, advances the
 * +0x1CC speed toward its ceiling. */
extern "C" void em_act_die_step(_ENEMY_WORK* self)
{
    if (em_die_ck(self) != 0) {
        return;
    }
    if (self->field_0x834 == 1) {
        em_attack_start(self);
    }
    if (self->grip_0x328.clung_0x10 == 1) {
        f32 speed = self->field_0x1CC;

        if (speed < 1.4f) {
            f32 next = speed;

            next += 0.01f;

            self->field_0x1CC = next;
            if (next > 1.4f) {
                self->field_0x1CC = 1.4f;
            }
        }
    }
}

/* Arms the +0x06 action on the first step, then ends the step when the action's frame check passes. */
extern "C" void em_act_arm_mot1(_ENEMY_WORK* self)
{
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

/* Arms the +0xC9 action (with the two effect resets that precede it), then ends the step. */
extern "C" void em_act_arm_mot201(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 201, 6, 0);
        em_ground_snap(self);
        break;
    case 1:
        em_ground_snap(self);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the two step functions this band holds. */
extern "C" void em_act_step_0(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot1(self);
        break;
    case 1:
        em_act_arm_mot1(self);
        break;
    case 2:
        em_act_arm_mot1(self);
        break;
    case 3:
        em_act_arm_mot201(self);
        break;
    }
}

/* Arms the +0x02 action on the first step, then ends the step when the action's frame check passes. */
extern "C" void em_act_arm_mot2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the action matching the caller's mode on the first step, then ends the step when the action's
 * frame check passes; mode 1 first re-arms the record's own sub-state. */
extern "C" void em_act_arm_mot_mode(_ENEMY_WORK* self, u32 action)
{
    if ((action & 0xFF) == 1) {
        em_fx_flag2_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        switch ((u8)action) {
        case 0:
            action = 3;
            break;
        case 1:
            action = 4;
            break;
        case 2:
            action = 6;
            break;
        case 3:
            action = 9;
            break;
        }
        em_mot_set(self, action, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Advances the record's two motion counters, then arms the +0x0B action or ends the step. */
extern "C" void em_act_arm_mot11(_ENEMY_WORK* self)
{
    em_motion_timer_arm(self);
    em_fx_flag_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 11, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_attack_done_set(self);
        }
        break;
    }
}

/* Advances the record's motion counters and scans the per-attacker values for one still inside the
 * band, handing the first such attacker to the damage handler. */
extern "C" void em_act_arm_mot1_hit1_1(_ENEMY_WORK* self)
{
    em_motion_timer_arm(self);
    em_fx_flag_set(self);
    em_shake_req_set(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_height_set(self, -100.0f);
        self->timer_0x020 = 0;
        break;
    case 1: {
        u32 max;
        u8 i;
        _PLW* work;

        em_height_set(self, -100.0f);
        if ((self->timer_0x020 & 0x1F) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++, work++) {
                f32 value;

                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (work->field_0x3B0 == 0) {
                    continue;
                }
                value = self->frames_0x454[i];
                if (value > 1000.0f) {
                    continue;
                }
                if (value < 0.0f) {
                    continue;
                }
                em_state_set(self, 1, 1);
                return;
            }
        }
        self->timer_0x020 += 1;
        break;
    }
    }
}

/* Arms the +0x14 action with a 20-frame frame check, then ends the step when that check passes. */
extern "C" void em_act_arm_mot1_f20(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set_blend(self, 1, 20, 0, 1);
        break;
    case 1:
        if (em_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Dispatches the record's sub-state: each arm is one of this band's step functions, and the table
 * MWCC emits for the eight arms is this unit's own `.data` (0x805DFC9C, 8 entries). */
extern "C" void em_act_step_1(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot2(self);
        break;
    case 1:
        em_act_arm_mot_mode(self, 0);
        break;
    case 2:
        em_act_arm_mot_mode(self, 1);
        break;
    case 3:
        em_act_arm_mot_mode(self, 2);
        break;
    case 4:
        em_act_arm_mot_mode(self, 3);
        break;
    case 5:
        em_act_arm_mot11(self);
        break;
    case 6:
        em_act_arm_mot1_hit1_1(self);
        break;
    case 7:
        em_act_arm_mot1_f20(self);
        break;
    }
}

/* Arms the +0x05 action with a speed factor from the record's own two low bits and a 150-frame
 * countdown, then ends the step when the frame check passes or the countdown runs out. */
extern "C" void em_act_arm_mot5(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 4, 0);
        em_mot_speed_set_ff(1.0f + 0.1f * (f32)(self->bits_0x1EC & 3), self);
        em_approach_start(self, -200.0f, 0);
        self->timer_0x020 = 150;
        break;
    case 1:
        if (em_approach_step(self, 0, 128) == 1 || --self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        break;
    }
}

/* The same step as `em_act_arm_mot5` for the +0x07 action, with its own speed factor and 120 frames. */
extern "C" void em_act_arm_mot7(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 7, 4, 0);
        em_mot_speed_set_ff(4.0f + 0.2f * (f32)(self->bits_0x1EC & 3), self);
        em_approach_start(self, -200.0f, 0);
        self->timer_0x020 = 120;
        break;
    case 1:
        if (em_approach_step(self, 0, 128) == 1 || --self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the +0x0C action and waits for its 512-frame check. */
extern "C" void em_act_arm_mot12(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 12, 4, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 512) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the +0x0D action and waits for its 1024-frame check. */
extern "C" void em_act_arm_mot13(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 13, 4, 0);
        break;
    case 1:
        if (em_turn_to_target(self, 1024) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the +0x0A action: its 2048-frame check runs first, then the action's own frame check. */
extern "C" void em_act_arm_mot10(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 10, 4, 0);
        break;
    case 1:
        em_turn_to_target(self, 2048);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Arms the +0x72 action with a 120-frame countdown, then ends the step when the countdown runs out. */
extern "C" void em_act_arm_mot114(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 114, 4, 0);
        self->timer_0x020 = 120;
        break;
    case 1:
        if (--self->timer_0x020 <= 0) {
            em_action_finish(self);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the six step functions above. */
extern "C" void em_act_step_2(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot5(self);
        break;
    case 1:
        em_act_arm_mot7(self);
        break;
    case 2:
        em_act_arm_mot12(self);
        break;
    case 3:
        em_act_arm_mot13(self);
        break;
    case 4:
        em_act_arm_mot10(self);
        break;
    case 5:
        em_act_arm_mot114(self);
        break;
    }
}

/* The +0xCA action's step: the two resets and the motion setter first, then the same frame check. */
extern "C" void em_act_arm_mot202(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 202, 6, 0);
        em_ground_snap(self);
        break;
    case 1:
        em_ground_snap(self);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* Steps the +0xC9 action and, every sixteenth step, hands the first attacker close enough to the
 * record's position to the damage handler. */
extern "C" void em_act_arm_mot201_hit7_2(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 201, 0, 0);
        em_ground_snap(self);
        self->timer_0x020 = 0;
        break;
    case 1: {
        u32 max;
        u8 i;
        _PLW* work;

        em_ground_snap(self);
        if ((self->timer_0x020 & 0xF) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++, work++) {
                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (calcVecDistXZ((VEC3*)&work->vec_0x03C, &self->pos) > 800.0f) {
                    continue;
                }
                em_state_set(self, 7, 2);
                return;
            }
        }
        self->timer_0x020 += 1;
        break;
    }
    }
}

/* Arms the +0xCB/+0xCC action - the mode picks between them - then ends the step at the frame check. */
extern "C" void em_act_arm_mot203_204(_ENEMY_WORK* self, u32 action)
{
    if ((u8)action == 1) {
        em_fx_flag2_set(self);
    }
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, (u16)(203 + ((action & 0xFF) == 1)), 0, 0);
        em_ground_snap(self);
        break;
    case 1:
        em_ground_snap(self);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* Arms the +0xCD action with a 120-frame countdown, then ends the step at the frame check. */
extern "C" void em_act_arm_mot205(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 205, 4, 0);
        em_approach_start(self, -100.0f, 0);
        self->timer_0x020 = 120;
        em_ground_snap(self);
        break;
    case 1:
        em_ground_snap(self);
        if (em_approach_step(self, 0, 64) == 1 || --self->timer_0x020 <= 0) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* Arms the +0xCF action and waits for its 1024-frame check. */
extern "C" void em_act_arm_mot207(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 207, 4, 0);
        em_ground_snap(self);
        break;
    case 1:
        em_ground_snap(self);
        if (em_turn_to_target(self, 1024) == 1) {
            em_action_finish_fall(self);
        }
        break;
    }
}

/* Arms the +0xD1 lurk and, every sixteenth step, looks for a player to ambush: none while the companion holds a
 * torch close by or a targeted player in range holds one, otherwise the nearest sighted player in range is reported
 * and the leap (7/3) is started; the effect placement runs at the end of every step. */
extern "C" void em026_act3_lurk(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set_ck(self, 209, 4, 0);
        self->timer_0x020 = 0;
        self->field_0x38B = 1;
        break;
    case 1:
        if ((self->timer_0x020 & 0xF) == 0) {
            s32 torch = 0;
            u32 found = 0;
            struct _AINPC_W* companion = &ainpc_w;

            if (companion->active != 0 && self->area_no == companion->field_0x1A4 &&
                ai_torch_ck(companion) == 1 && vec3_dist_sq(&self->pos, &companion->vec_0x178) <= 1000000.0f) {
                torch = 1;
            }
            if (torch == 0) {
                f32 nearest = 800.0f;
                u32 max = get_move_work_max(2);
                _PLW* pl = (_PLW*)get_move_work_adrs(2);
                u8 i;

                for (i = 0; i < (u16)max; i++, pl++) {
                    if (pl == NULL || pl->slot_active == 0 || em_pl_target_ck(self, pl) == 0) {
                        continue;
                    }
                    if (self->frames_0x454[i] > 700.0f || self->frames_0x454[i] < 0.0f) {
                        continue;
                    }
                    if (pl_torch_ck((_ENEMY_WORK*)pl) == 1) {
                        found = 0;
                        break;
                    }
                    if (em_sight_player_ck2(self, i) != 0) {
                        if (self->frames_0x454[i] < nearest) {
                            nearest = self->frames_0x454[i];
                            em_reaction_req(self, 1, 2, i);
                        }
                        found = 1;
                    }
                }
            }
            if (found == 1) {
                em_state_set(self, 7, 3);
            }
        }
        self->timer_0x020 += 1;
        break;
    }
    em_eff_offset_set(self);
}

/* Arms the +0xD2 action, and on the frame check hands the target to the damage handler; the effect
 * placement runs at the end of every step. */
extern "C" void em_act_arm_mot210_hit3_10(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 210, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 3, 10);
        }
        break;
    }
    em_eff_offset_set(self);
}

/* Steps the +0xC9 action and, every sixteenth step, hands the first attacker close enough to the
 * record's position to the damage handler. */
extern "C" void em_act_arm_mot201_hit7_6(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 201, 4, 0);
        self->timer_0x020 = 0;
        break;
    case 1: {
        u32 max;
        u8 i;
        _PLW* work;

        if ((self->timer_0x020 & 0xF) == 0) {
            max = get_move_work_max(2);
            work = (_PLW*)get_move_work_adrs(2);
            for (i = 0; i < (u16)max; i++, work++) {
                if (work == NULL) {
                    continue;
                }
                if (work->slot_active == 0) {
                    continue;
                }
                if (calcVecDistXZ((VEC3*)&work->vec_0x03C, &self->pos) > 800.0f) {
                    continue;
                }
                em_state_set(self, 7, 6);
                return;
            }
        }
        self->timer_0x020 += 1;
        break;
    }
    }
}

/* Arms the +0xCB action, then hands the target to the damage handler at the frame check. */
extern "C" void em_act_arm_mot203_hit3_20(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 203, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 3, 20);
        }
        break;
    }
}

/* Dispatches the record's sub-state onto the band's step functions; unlisted states do nothing. */
extern "C" void em_act_step_3(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_act_arm_mot202(self);
        break;
    case 1:
        em_act_arm_mot201_hit7_2(self);
        break;
    case 2:
        em_act_arm_mot203_204(self, 0);
        break;
    case 3:
        em_act_arm_mot203_204(self, 1);
        break;
    case 4:
        em_act_arm_mot205(self);
        break;
    case 5:
        em_act_arm_mot207(self);
        break;
    case 10:
        em026_act3_lurk(self);
        break;
    case 11:
        em_act_arm_mot210_hit3_10(self);
        break;
    case 20:
        em_act_arm_mot201_hit7_6(self);
        break;
    case 21:
        em_act_arm_mot203_hit3_20(self);
        break;
    }
}

/* 1 while the record's seat is armed and its player is close enough for the aim step, -1 while the
 * seat is armed but the aim is refused, 0 when there is no seat to aim. */
extern "C" s8 em_seat_aim_ck(_ENEMY_WORK* self)
{
    if (self->field_0xA16 != 0xFF && self->field_0xA38 == 0) {
        if (em_act_face_away(self, self->plw_0xA34) == 1) {
            return 1;
        }
        return -1;
    }
    return 0;
}

/* Action 7, step 0: winds up (+0x2E, +0x2F), leaps (+0x30) with the hit window armed and, while airborne, clings
 * to an aimable seat's player (7/4) or lands (+0x31). */
extern "C" void em026_act7_leap(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        self->state_0x006 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x2E, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x2F, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_fall_height_get(self);
            em_fall_start(self);
            em_mot_set(self, 0x30, 0, 0);
            em_hit_window_set(self, 0, 1, 2);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = 25.0f;
            self->offset_0x30C.vec_0x310.z = 20.0f;
            self->vec_0x31C.y = -3.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 3: {
        CancelFade(self);
        if (self->state_0x006 == 0) {
            s8 aim = em_seat_aim_ck(self);

            if (aim != 0) {
                self->state_0x006 = 1;
                em_hit_window_clear(self, 0);
                if (aim == 1) {
                    em_state_set(self, 7, 4);
                    break;
                }
            }
        }
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x31, 0, 0);
        }
        break;
    }
    case 4:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7, step 1: arms the +0x34 spit and, at its frame 60, launches the shell effect record. */
extern "C" void em026_act7_spit(_ENEMY_WORK* self)
{
    EmSpawnRec rec;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x34, 4, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            nw4r::math::VEC3 v;

            rec.id = 7;
            copyVec3(&rec.pos, setVec3(&v, 0.0f, -15.0f, -25.0f));
            rec.field_0x10 = 0;
            rec.field_0x12 = 0xF000;
            rec.field_0x14 = 0;
            eft007_part_spawn(self, 25, rec.field_0x12, 0);
            ((Em026ShellRecJobFn)shell_set_func_ptr->method_0x3C)(self, &rec, 12);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7, steps 2 and 6: drops from the ceiling (mode 1 skips the ground snap) - waits 60 frames on the +0xC9
 * motion with its dust and cry, then falls (+0x3C, +0x3D) with the hit window armed and lands (+0x3E) with its
 * landing dust. */
extern "C" void em026_act7_drop(_ENEMY_WORK* self, u8 mode)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0xC9, 6, 0);
        if (mode != 1) {
            em_ground_snap(self);
        }
        self->timer_0x020 = 60;
        setVector3(&v, 0.0f, 0.0f, 0.0f);
        eft_em_spawn(self, 0x4F, 3, &v, 1.0f);
        if (self->area_no == get_now_areano()) {
            se_req_pos_ps(self->se_0xB14, 0x16, 2, &self->pos);
        }
        break;
    case 1:
        if (mode != 1) {
            em_ground_snap(self);
        }
        if (--self->timer_0x020 <= 0) {
            self->state += 1;
            em_mot_set(self, 0x3C, 2, 0);
        }
        break;
    case 2:
        if (mode != 1) {
            em_ground_snap(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x3D, 0, 0);
            em_hit_window_set(self, 0, 1, 2);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = -35.0f;
            CancelFade(self);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = -10.0f;
            self->offset_0x30C.vec_0x310.z = 0.0f;
            self->vec_0x31C.y = -3.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 3:
        CancelFade(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x3E, 0, 0);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0x18, 0, 0.8f);
            } else {
                eft009_spawn_at_joint(self, 3, 9, 0, 0.8f);
            }
        }
        break;
    case 4:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7, step 3: winds up (+0x36) with the effect placed, lunges forward (+0x37) turned half round with the hit
 * window armed and, while airborne, clings to an aimable seat's player (7/4) or lands (+0x38). */
extern "C" void em026_act7_lunge(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        self->state_0x006 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x36, 2, 0);
        em_eff_offset_set(self);
        break;
    case 1:
        em_eff_offset_set(self);
        if (em_mot_end_ck(self) == 1) {
            self->state += 1;
            em_mot_set(self, 0x37, 0, 0);
            em_hit_window_set(self, 0, 1, 2);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = -30.0f;
            self->offset_0x30C.vec_0x310.z = 60.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
            em_move_offset_apply(self);
            em_turn_to_target_c1(self, 0x8000);
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->offset_0x30C.vec_0x310.z = 40.0f;
            self->vec_0x31C.y = -3.0f;
            rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        }
        break;
    case 2: {
        CancelFade(self);
        if (self->state_0x006 == 0) {
            s8 aim = em_seat_aim_ck(self);

            if (aim != 0) {
                self->state_0x006 = 1;
                em_hit_window_clear(self, 0);
                if (aim == 1) {
                    em_state_set(self, 7, 4);
                    break;
                }
            }
        }
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x38, 0, 0);
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

/* Action 7, step 4: clings to the seat's player - rides the player's joint at the grip offset for up to 420 frames
 * (360 fewer once the player guards), letting go (7/5) when the time is up, the player shakes it off or the joint
 * moves too far; the grip latch is set once it has held 150 frames. */
extern "C" void em026_act7_cling(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    EmGripBlock* grip;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    grip = &self->grip_0x328;
    em_fx_flag_set(self);
    em_motion_timer_arm(self);
    switch (self->state) {
    case 0:
        self->state += 1;
        grip->cling_frames_0x12 = 0;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x32, 0, 0);
        em_flag_0x8AF_set(self);
        em_guard_set(self);
        self->timer_0x020 = 420;
        pl_joint_mtx_get(&((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6], grip->joint_0x0C, &mtx);
        copyVec3(&pos, &grip->offset);
        mulVecMatAddTrans(&pos, &mtx);
        copyVec3(&self->pos, &pos);
        break;
    case 1: {
        _PLW* pl = &((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6];
        s32 lost;
        f32 dist;

        pl_joint_mtx_get(pl, grip->joint_0x0C, &mtx);
        copyVec3(&pos, &grip->offset);
        mulVecMatAddTrans(&pos, &mtx);
        dist = vec3_dist_sq(&self->pos, &pos);
        lost = 0;
        if (dist < 10000.0f) {
            copyVec3(&self->pos, &pos);
        } else {
            lost = 1;
        }
        em_gauge_period_step_v(self, 1.0f, 5, 30);
        if (pl_guard_latch_ck(pl) == 1) {
            self->timer_0x020 -= 360;
        }
        if (--self->timer_0x020 <= 0 || pl_shake_off_ck(pl) == 1 || lost != 0) {
            em_eff_offset_clr(self);
            em_state_set(self, 7, 5);
        }
        if (grip->clung_0x10 == 0) {
            if (grip->cling_frames_0x12 < 150) {
                grip->cling_frames_0x12 += 1;
            } else {
                grip->clung_0x10 = 1;
                self->run_0xB04.flags = 1;
            }
        }
        break;
    }
    }
}

/* Action 7, step 5: shaken off - thrown back from the player's heading (+0x6C), falls and lands (+0x66). */
extern "C" void em026_act7_shaken_off(_ENEMY_WORK* self)
{
    em_fx_flag_set(self);
    switch (self->state) {
    case 0: {
        _PLW* pl;

        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0x6C, 4, 0);
        pl = &((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6];
        self->field_0x1C0 += pl->field_0x058;
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.z = -20.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        CancelFade(self);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.y = 0.0f;
        self->offset_0x30C.vec_0x310.z = -20.0f;
        self->vec_0x31C.y = -5.0f;
        rotVecY(&self->offset_0x30C.vec_0x310, self->field_0x1C0);
        break;
    }
    case 1:
        CancelFade(self);
        em_fall_height_get(self);
        if (em_ground_ck(self) == 1) {
            self->state += 1;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x66, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Dispatches action 7's sub-state onto its step functions. */
extern "C" void em026_act7_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em026_act7_leap(self);
        break;
    case 1:
        em026_act7_spit(self);
        break;
    case 2:
        em026_act7_drop(self, 0);
        break;
    case 3:
        em026_act7_lunge(self);
        break;
    case 4:
        em026_act7_cling(self);
        break;
    case 5:
        em026_act7_shaken_off(self);
        break;
    case 6:
        em026_act7_drop(self, 1);
        break;
    }
}

/* The sound programs' offsets (`.sdata`): the pair the 104 records shift by and the 0x0E entry's pair. */
EmSeOffset em026_se_shift_a = {-50.0f, -10.0f};
EmSeOffset em026_se_shift_b = {2.5f, 0.0f};

/* The per-sub-state sound programs `em026_act10_se_play`/`em026_act11_se_play` hand the table walker (`.data`; each
 * table is named after the first sub-state that plays it). */
EmSeRecord em026_se_tbl_0_rec0 = {100, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_tbl_0[2] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_23_rec0 = {101, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_tbl_23_rec2 = {108, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em026_vec_pair};
EmSeRecord em026_se_tbl_23_rec4 = {102, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_tbl_23[6] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_23_rec0},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em026_se_tbl_23_rec2},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em026_se_tbl_23_rec4},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_5_rec0 = {103, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_tbl_5_rec1 = {104, 0, 0, 0, 0, {0, 2, 1, 0}, &em026_se_shift_a};
EmSeRecord em026_se_tbl_5_rec3 = {105, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_tbl_5[5] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_5_rec0},
    {0x00, {0, 0, 0}, &em026_se_tbl_5_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em026_se_tbl_5_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_122_rec0 = {115, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_tbl_122[2] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_122_rec0},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_123_rec0 = {14, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em026_se_tbl_123[2] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_123_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_124_rec0 = {114, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em026_se_tbl_124[2] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_124_rec0},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_141_rec0 = {1, 0, 20, 0, 1, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_tbl_141[2] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_141_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_125_rec1 = {115, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_tbl_125_rec2 = {14, 0, 0, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em026_se_tbl_125[4] = {
    {0x0E, {0, 0, 0}, (const EmSeRecord*)&em026_se_shift_b},
    {0x00, {0, 0, 0}, &em026_se_tbl_125_rec1},
    {0x00, {0, 0, 0}, &em026_se_tbl_125_rec2},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_55_rec0 = {103, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_tbl_55_rec1 = {104, 0, 0, 0, 0, {0, 2, 1, 0}, &em026_se_shift_a};
EmSeRecord em026_se_tbl_55_rec3 = {105, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_tbl_55[5] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_55_rec0},
    {0x00, {0, 0, 0}, &em026_se_tbl_55_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em026_se_tbl_55_rec3},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_tbl_214_rec0 = {101, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_tbl_214_rec2 = {108, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em026_vec_pair};
EmSeRecord em026_se_tbl_214_rec4 = {102, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_tbl_214_rec6 = {114, 0, 4, 0, 0, {0, 4, 0, 0}, 0};
EmSeEntry em026_se_tbl_214[8] = {
    {0x00, {0, 0, 0}, &em026_se_tbl_214_rec0},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em026_se_tbl_214_rec2},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em026_se_tbl_214_rec4},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em026_se_tbl_214_rec6},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_alt_tbl_0_rec0 = {115, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_alt_tbl_0[2] = {
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_0_rec0},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_alt_tbl_5_rec0 = {101, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_alt_tbl_5_rec2 = {108, 0, 0, 0, 0, {0, 2, 2, 0}, (const EmSeOffset*)&em026_vec_pair};
EmSeRecord em026_se_alt_tbl_5_rec4 = {102, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_alt_tbl_5[6] = {
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_5_rec0},
    {0x0B, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_5_rec2},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_5_rec4},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em026_se_alt_tbl_14_rec0 = {103, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em026_se_alt_tbl_14_rec1 = {104, 0, 0, 0, 0, {0, 2, 1, 0}, &em026_se_shift_a};
EmSeRecord em026_se_alt_tbl_14_rec3 = {105, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em026_se_alt_tbl_14[5] = {
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_14_rec0},
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_14_rec1},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em026_se_alt_tbl_14_rec3},
    {0xFF, {0, 0, 0}, 0},
};
f32 em026_demo_key_a[52] = {
    2e+01f, -554.49f, -1710.73f,
    22.0f, -552.05f, -1709.82f,
    26.0f, -537.43f, -1704.31f,
    3e+01f, -520.64f, -1697.99f,
    34.0f, -518.34f, -1697.13f,
    38.0f, -516.32f, -1696.37f,
    42.0f, -514.98f, -1695.86f,
    46.0f, -513.58f, -1695.33f,
    5e+01f, -512.15f, -1694.8f,
    54.0f, -510.91f, -1694.33f,
    58.0f, -510.05f, -1694.01f,
    62.0f, -509.8f, -1693.91f,
    66.0f, -510.22f, -1694.07f,
    7e+01f, -510.61f, -1694.22f,
    74.0f, -510.72f, -1694.26f,
    78.0f, -510.5f, -1694.18f,
    8e+01f, -510.31f, -1694.11f,
    0.0f,
};
f32 em026_demo_key_b[168] = {
    8e+01f, -510.31f, 60.39f, -1694.11f,
    84.0f, -509.87f, 60.55f, -1693.94f,
    88.0f, -509.43f, 61.04f, -1693.78f,
    92.0f, -509.13f, 61.83f, -1693.66f,
    96.0f, -508.9f, 62.89f, -1693.57f,
    1e+02f, -508.71f, 64.13f, -1693.5f,
    104.0f, -508.53f, 65.41f, -1693.44f,
    108.0f, -508.36f, 66.16f, -1693.37f,
    112.0f, -508.16f, 66.51f, -1693.3f,
    116.0f, -507.92f, 66.61f, -1693.21f,
    1.2e+02f, -507.63f, 66.59f, -1693.1f,
    124.0f, -507.26f, 66.6f, -1692.96f,
    128.0f, -506.88f, 66.69f, -1692.81f,
    132.0f, -506.77f, 66.69f, -1692.77f,
    136.0f, -506.74f, 66.43f, -1692.76f,
    1.4e+02f, -506.48f, 65.8f, -1692.66f,
    144.0f, -505.73f, 64.89f, -1692.38f,
    148.0f, -504.55f, 63.96f, -1691.94f,
    152.0f, -503.21f, 63.2f, -1691.43f,
    156.0f, -501.77f, 62.73f, -1690.89f,
    1.6e+02f, -500.23f, 62.51f, -1690.31f,
    164.0f, -498.63f, 62.38f, -1689.71f,
    168.0f, -496.99f, 62.29f, -1689.09f,
    172.0f, -495.34f, 62.24f, -1688.47f,
    176.0f, -493.69f, 62.21f, -1687.85f,
    1.8e+02f, -492.07f, 62.18f, -1687.24f,
    184.0f, -490.52f, 62.17f, -1686.66f,
    188.0f, -489.05f, 62.16f, -1686.1f,
    192.0f, -487.71f, 62.15f, -1685.6f,
    196.0f, -486.54f, 62.13f, -1685.16f,
    2e+02f, -485.62f, 62.12f, -1684.81f,
    204.0f, -485.03f, 62.09f, -1684.59f,
    208.0f, -484.57f, 62.05f, -1684.42f,
    212.0f, -483.93f, 61.99f, -1684.18f,
    216.0f, -483.08f, 61.93f, -1683.86f,
    2.2e+02f, -482.01f, 61.85f, -1683.45f,
    224.0f, -480.7f, 61.78f, -1682.96f,
    228.0f, -479.13f, 61.71f, -1682.37f,
    232.0f, -477.33f, 61.64f, -1681.69f,
    236.0f, -475.33f, 61.58f, -1680.94f,
    2.4e+02f, -473.24f, 61.53f, -1680.15f,
    -1.0f, -473.24f, 61.53f, -1680.15f,
};

/* Action 10: plays the sound program of the current sub-state; any other sub-state ends the action. */
extern "C" void em026_act10_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play(self, em026_se_tbl_0, 0, 0);
        break;
    case 23:
        em_se_tbl_play(self, em026_se_tbl_23, 1, 23);
        break;
    case 5:
        em_se_tbl_play(self, em026_se_tbl_5, 1, 5);
        break;
    case 35:
        em_se_tbl_play(self, em026_se_tbl_5, 1, 35);
        break;
    case 55:
        em_se_tbl_play(self, em026_se_tbl_55, 1, 55);
        break;
    case 122:
        em_se_tbl_play(self, em026_se_tbl_122, 0, 122);
        break;
    case 123:
        em_se_tbl_play(self, em026_se_tbl_123, 0, 123);
        break;
    case 159:
        em_se_tbl_play(self, em026_se_tbl_141, 0, 159);
        break;
    case 160:
        em_se_tbl_play(self, em026_se_tbl_23, 1, 160);
        break;
    case 124:
        em_se_tbl_play(self, em026_se_tbl_124, 0, 124);
        break;
    case 141:
        em_se_tbl_play(self, em026_se_tbl_141, 0, 141);
        break;
    case 125:
        em_se_tbl_play(self, em026_se_tbl_125, 0, 125);
        break;
    case 142:
        em_se_tbl_play(self, em026_se_tbl_141, 0, 142);
        break;
    case 132:
        em_se_tbl_play(self, em026_se_tbl_5, 1, 132);
        break;
    case 158:
        em_se_tbl_play(self, em026_se_tbl_55, 1, 158);
        break;
    case 214:
        em_se_tbl_play(self, em026_se_tbl_214, 1, 214);
        break;
    case 215:
        em_se_tbl_play(self, em026_se_tbl_5, 1, 215);
        break;
    case 220:
        em_se_tbl_play(self, em026_se_tbl_55, 1, 220);
        break;
    default:
        em_action_finish(self);
        break;
    }
}

/* Action 11: plays the alternate sound program of the current sub-state (the sub-state 0 program by default). */
extern "C" void em026_act11_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_0, 0, 0);
        break;
    case 5:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_5, 1, 5);
        break;
    case 14:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_14, 1, 14);
        break;
    case 29:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_0, 0, 29);
        break;
    case 30:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_5, 1, 30);
        break;
    case 35:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_14, 1, 35);
        break;
    default:
        em_se_tbl_play_alt(self, em026_se_alt_tbl_0, 0, 0);
        break;
    }
}

/* Action 12, step 0: arms the +0x03 idle and ends the action with it. */
extern "C" void em026_act12_idle(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 3, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 12, step 1: drops in (+0xCB) on the ground, then starts 3/1. */
extern "C" void em026_act12_drop_hit1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0xCB, 0, 0);
        em_ground_snap(self);
        break;
    case 1:
        em_ground_snap(self);
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 3, 1);
        }
        break;
    }
}

/* Action 12, step 2: arms the +0xD2 motion, then starts 3/10. */
extern "C" void em026_act12_mot210_hit10(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0xD2, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 3, 10);
        }
        break;
    }
}

/* Action 12, step 3: drops in (+0xCB), then starts 3/20. */
extern "C" void em026_act12_drop_hit20(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_fall_height_get(self);
        em_fall_start(self);
        em_mot_set(self, 0xCB, 0, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_state_set(self, 3, 20);
        }
        break;
    }
}

/* Dispatches action 12's sub-state onto its step functions. */
extern "C" void em026_act12_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        em026_act12_idle(self);
        break;
    case 1:
        em026_act12_drop_hit1(self);
        break;
    case 2:
        em026_act12_mot210_hit10(self);
        break;
    case 3:
        em026_act12_drop_hit20(self);
        break;
    }
}

/* Action 13, demo A: waits for demo frame 0x71E, then walks in (+0x04) at the demo position with the step dust at
 * frame 14, and blends into +0x05 at demo frame 0x804. */
extern "C" void em026_act13_demo_a(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(0x71E) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 4, 0, 0);
            em_demo_pos_set(self, -527.0708f, 36.0352f, -1683.0796f);
            em_demo_rot_set(self, 0.0f, 165.75f, 0.0f);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 20.0f);
            eft_em_spawn(self, 0xC7, 7, &v, 1.0f);
        }
        if (em_demo_time_ck(0x804) == 1) {
            self->state += 1;
            em_mot_set_blend(self, 5, 10, 0, 1);
        }
        break;
    }
}

/* Action 13, the end of demo A: the +0x05 motion at its last demo position. */
extern "C" void em026_act13_demo_a_end(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 0, 0);
        em_demo_pos_set(self, -483.8582f, 36.0352f, -1853.2297f);
        em_demo_rot_set(self, 0.0f, 165.75f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}


/* Action 13, demo B: waits for demo frame 0x61E (restarting its frame count), steps along the two key tracks from
 * demo frame 0x632 with the step dust at frame 14, and turns into +0x09 at demo frame 0x710. */
extern "C" void em026_act13_demo_b(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;
    s32 frame;

    VEC3_ctor(&v);
    frame = ++self->timer_0x020;
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(0x61E) == 1) {
            self->state += 1;
            self->timer_0x020 = 0;
        }
        break;
    case 2:
        if (em_demo_time_ck(0x632) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 4, 0, 0);
            em_demo_pos_set(self, -554.49f, 60.39f, -1710.73f);
            em_demo_key_apply(self, self->timer_0x020, em026_demo_key_a, NULL, 5, 0);
            em_demo_rot_set(self, 0.0f, 69.38f, 0.0f);
        }
        break;
    case 3:
        if (frame * 2 < 80) {
            em_demo_key_apply(self, frame, em026_demo_key_a, NULL, 5, 0);
        } else {
            em_demo_key3_apply(self, frame, em026_demo_key_b, NULL);
        }
        if (em_frame_check(self, 0, 14.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 20.0f);
            eft_em_spawn(self, 0xC7, 7, &v, 1.0f);
        }
        if (em_demo_time_ck(0x710) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_mot_set(self, 9, 0, 0x1A);
            em_demo_pos_set(self, -459.8198f, 14.8009f, -1862.3315f);
            em_demo_rot_set(self, 0.0f, -69.375f, 0.0f);
        }
        break;
    }
}

/* Action 13, the end of demo B: the +0x05 motion at its last demo position. */
extern "C" void em026_act13_demo_b_end(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 0, 0);
        em_demo_pos_set(self, -459.8198f, 14.8009f, -1862.3315f);
        em_demo_rot_set(self, 0.0f, -69.375f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 13, demo C: waits for demo frame 0x710, walks in (+0x04), then blends into +0x06 at 0x71A and +0x07 at
 * 0x728. */
extern "C" void em026_act13_demo_c(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, NULL);
        break;
    case 1:
        if (em_demo_time_ck(0x710) == 1) {
            self->state += 1;
            em_demo_enable(self);
            em_mot_set(self, 4, 0, 0xE2);
            em_demo_pos_set(self, -416.0534f, 12.5995f, -1751.6676f);
            em_demo_rot_set(self, 0.0f, 110.25f, 0.0f);
        }
        break;
    case 2:
        if (em_demo_time_ck(0x71A) == 1) {
            self->state += 1;
            em_mot_set_blend(self, 6, 10, 12, 1);
        }
        break;
    case 3:
        if (em_demo_time_ck(0x728) == 1) {
            self->state += 1;
            em_mot_set_blend(self, 7, 10, 0, 1);
        }
        break;
    }
}

/* Action 13, the end of demo C: the +0x05 motion at its last demo position. */
extern "C" void em026_act13_demo_c_end(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state += 1;
        em_move_mode_set(self, 0);
        em_mot_set(self, 5, 0, 0);
        em_demo_pos_set(self, -301.6223f, 12.5995f, -1793.8835f);
        em_demo_rot_set(self, 0.0f, 110.25f, 0.0f);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Dispatches action 13's sub-state onto the three demos and their ends. */
extern "C" void em026_act13_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 2:
        em026_act13_demo_a(self);
        break;
    case 3:
        em026_act13_demo_a_end(self);
        break;
    case 4:
        em026_act13_demo_b(self);
        break;
    case 5:
        em026_act13_demo_b_end(self);
        break;
    case 6:
        em026_act13_demo_c(self);
        break;
    case 7:
        em026_act13_demo_c_end(self);
        break;
    }
}

/* The program table's action run: dispatches the current action's sub-state machine. */
extern "C" void em026_action_run(_ENEMY_WORK* self)
{
    switch (self->action) {
    case 0:
        em_act_step_0(self);
        break;
    case 1:
        em_act_step_1(self);
        break;
    case 2:
        em_act_step_2(self);
        break;
    case 3:
        em_act_step_3(self);
        break;
    case 7:
        em026_act7_dispatch(self);
        break;
    case 10:
        em026_act10_se_play(self);
        break;
    case 11:
        em026_act11_se_play(self);
        break;
    case 12:
        em026_act12_dispatch(self);
        break;
    case 13:
        em026_act13_dispatch(self);
        break;
    }
}

/* Per-frame motion events: keeps the cling request on the gripped player, drops the lurk flag outside 3/10, and on
 * motion 11 raises the landing dust at frame 60, the step dust every 20 frames from 90 to 190 and the shake at
 * frame 128. */
extern "C" void em026_motion_events(_ENEMY_WORK* self)
{
    if (em_flag_0x8AF_ck(self) == 1) {
        pl_cling_req_set(&((_PLW*)get_move_work_adrs(2))[self->owner_slot_0x1F6], self->group);
    }
    if (em_act_ck(self, 3, 10) == 0 && self->field_0x38B == 1) {
        self->field_0x38B = 0;
    }
    if ((s32)em_get_mot_no(self) == 11) {
        if (em_after_frame_check(self, 0, 60.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0xC, 0, 0.5f);
            } else {
                eft009_spawn_at_joint(self, 3, 2, 0, 0.5f);
            }
        }
        if (em_after_frame_check(self, 0, 90.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 110.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 130.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 150.0f, 0.0f) == 1 ||
            em_after_frame_check(self, 0, 170.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_joint(self, 3, 0xC, 0, 0.5f);
            } else {
                eft009_spawn_at_joint(self, 3, 8, 0, 0.5f);
            }
        }
        if (em_after_frame_check(self, 1, 128.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
    }
}

/* The program table's empty TEV colour slot. */
extern "C" void em026_tev_color_update(void)
{
}

/* The program table's state query: kind 0 the grip latch, kind 1 the attack timer. */
extern "C" u8 em026_state_query(_ENEMY_WORK* self, u8 kind)
{
    switch (kind) {
    case 0:
        if (self->grip_0x328.clung_0x10 == 1) {
            return 1;
        }
        break;
    case 1:
        if (self->field_0x011 != 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The program's cry: on sub-states 0 and 29 plays sound 0x39 at the body when it is in the current area. */
extern "C" u32 em026_cry_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
    case 29:
        if (self->area_no == get_now_areano()) {
            se_req_pos_ps(self->se_0xB14, 0x39, 2, &self->pos);
        }
        break;
    }
    return 0;
}

/* The program table's death action: the action the area kind asks for, then back to the ground record's position
 * (the kind-1 effect ground otherwise). */
extern "C" void em026_death_action_get(_ENEMY_WORK* self, u8* action, u8* sub)
{
    EmGroundRec rec;

    em_ground_rec_clear(&rec);
    switch (self->field_0x00A) {
    case 0:
        em_move_mode_set(self, 0);
        *action = 12;
        *sub = 0;
        break;
    case 2:
        em_fall_height_get(self);
        em_fall_start(self);
        *action = 12;
        *sub = 1;
        break;
    case 3:
        em_fall_height_get(self);
        em_fall_start(self);
        *action = 12;
        *sub = 2;
        break;
    case 1:
        em_move_mode_set(self, 0);
        *action = 1;
        *sub = 6;
        break;
    case 6:
        em_fall_height_get(self);
        em_fall_start(self);
        *action = 12;
        *sub = 3;
        break;
    }
    if (self->field_0x00A == 1) {
        em_eff_ground_set(self);
    } else if (em_ground_rec_find(self->field_0x01A, &rec) == 1) {
        copyVec3(&self->pos, &rec.pos_0x08);
        eft_rot_vec_copy((_CP_VECTOR*)&self->field_0x1BC, (_CP_VECTOR*)&rec.field_0x14);
    }
}

/* The program table's empty re-arm slot. */
extern "C" void em026_rearm_noop(void)
{
}

/* The program table's state-clear test: 1 while the +0x1E2 byte is clear or the 0x8AE flag is set. */
extern "C" u32 em026_state_clear_ck(_ENEMY_WORK* self)
{
    if (self->field_0x1E2 == 0 || em_flag_0x8AE_ck(self) == 1) {
        return 1;
    }
    return 0;
}

/* The program table's area-keep slot: resets the mini work's kept position, then keeps the given position (flag 2)
 * and the source's rotation (flag 4), and marks the block valid. */
extern "C" void em026_area_keep_set(_ENEMY_MINI_WORK* mini, nw4r::math::VEC3* pos, Em026KeepSrc* src, u8 flags)
{
    EmRunPos* keep = (EmRunPos*)mini->data_0x34;
    nw4r::math::VEC3 tmp;

    copyVec3(&keep->pos, setVec3(&tmp, 0.0f, 0.0f, 0.0f));
    keep->angle = 0;
    if (flags & 2) {
        copyVec3(&keep->pos, pos);
    }
    if (flags & 4) {
        keep->angle = src->rot_0x04;
    }
    keep->valid_0x0F |= 1;
}

/* 0x8032FFEC (0x8): Sets the accessor's flag word to 2 once it is installed. */
void EmActStepUserData::Attach()
{
    em_res_user_data_flag_set((struct _ENEMY_WORK*)this, 2);
}

/* 0x8032FFF4 (0xD8): While the enemy clings (flag 0x8AF), for an admitted item of node 1 replaces the joint's
 * rotation with the gripped player's joint rotation. */
void EmActStepUserData::CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                                      nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item)
{
    nw4r::math::MTX34 joint;
    nw4r::math::MTX34 player;
    _ENEMY_WORK* work;

    MTX34_ctor(&joint);
    MTX34_ctor(&player);
    work = this->work;
    if (em_flag_0x8AF_ck(work) == 1 && (s32)item->field_0x04 == 0xFF && node == 1) {
        joint_mtx_load((struct MtxHolder*)pManip, &joint);
        pl_joint_mtx_get(&((_PLW*)get_move_work_adrs(2))[work->owner_slot_0x1F6], work->grip_0x328.joint_0x0C, &player);
        mtx34_concat_assign(&player, &joint);
        copyMat33(&joint, &player);
        joint_mtx_store((struct MtxHolder*)pManip, &joint);
    }
}

/* 0x803300CC (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
EmActStepUserData::~EmActStepUserData()
{
}


extern "C" {

/* The unit's `.bss`: the vector pair MWCC's `__sinit` seeds (a point and its step); three sound records point at
 * it. */
Em026VecPair em026_vec_pair(17.0f, -20.0f, -2.38f, 0.11f);

} /* extern "C" */
