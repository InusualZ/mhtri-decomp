/*
 * enemy/em009_act.cpp - enemy 009's program: the `em009_prog_tbl` entry points (work clear, init, frame update,
 *   action dispatch, motion transitions, event hook, motion events, state query, death action, action selection),
 *   the per-action state steps the `state_sub` dispatchers select, the two sound programs, the scripted entrance,
 *   the effect router and the user-data helper's constructor and release.
 * RANGE. .text 0x803868DC-0x8038EC44 (64 functions); extab 0x8001803C-0x800181C4, extabindex
 *   0x80037EC0-0x8003810C, .rodata 0x80570B20-0x80570BA0, .data 0x805EF990-0x805F0CB8 (`em009_prog_tbl` first),
 *   .sdata2 0x8079BFF8-0x8079C298.  Left edge: the 0.0 pool entry repeats at 0x8079BFF8, first read by
 *   `em009_init`; right edge: `Em009UserData::~Em009UserData` closes the TU (the class table at 0x805F0C88 names it).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi`+`cmpwi` forms),
 *   `#pragma pool_data off` and `#pragma fp_contract off` (no fused multiply-add in the target).
 * NAMES. The file name follows the runtime dump's `em009_prog_tbl`, which opens the TU's `.data`.
 *   GUESS (the `Em008ProgTbl` slot each fills): em009_work_clear, em009_init, em009_frame_update,
 *   em009_action_dispatch, em009_motion_next, em009_event_notify, em009_motion_events, em009_tev_color_update,
 *   em009_state_query, em009_death_action_get, em009_cooldown_clear, em009_part_broken_ck, em009_action_select.
 *   GUESS (dispatcher slot): em009_act<action>_<first sub-state> and em009_act<action>_dispatch,
 *   em009_act10_se_play, em009_act11_se_play; GUESS (from the bodies): em009_alt_mode_effect, em009_effect_spawn,
 *   `Em009UserData` (the class whose table is 0x805F0C88).
 *   GUESS (data): em009_slide_curve, em009_pick_tbl, em009_route_points, em009_turn_seq_0/1,
 *   em009_demo_keys3/_a/_a_time/_b, em009_se_seq_<sub-state> (the sub-state that first plays each table) with their
 *   em009_se_rec/pos records, the `Em009ProgTbl` slot names.
 * RESIDUALS. `em009_act2_11`: retail computes the motion select into r4 (`addi r4,r4,0x1f;
 *   clrlwi r4,r4,16`), ours into r0 (a u16 callee view, a local, unsigned constants and the inverted test all leave
 *   it).  `.sdata2` is 0x29C against the claim's 0x2A0 (the trailing word is the section's 8-byte alignment pad).
 *   flipcheck: `em009_act2_11`.
 * SHAPES. `Em009UserData` derives from `ResUserDataAc` (`enemy/res_user_data_ac.h`), so MWCC emits its table; it is
 *   allocated with `operator new` and built with placement `new` (a new-expression adds a `__dl__FPv` landing pad,
 *   0x10 of extab, retail does not have).  A case-level `return` that retail lays out as a `break` is written
 *   `break`; one-case signed compares are
 *   `switch`es; range tests retail compiles as a compare tree are `switch`es with case lists (`em009_effect_spawn`,
 *   `em009_act7_5`'s slide limit, `em009_act6_10`'s reaction).  A product of calls is one expression with the calls
 *   in place (`0.45f * (chg * (curve * base))`); the shell-set slots are called through typed slot pointers without
 *   the table argument.  The data is defined between the functions in retail order (the jump tables fall between
 *   them), the `.sdata2` pool from literals.
 */

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/enemy_control.h"
#include "ef/eft009.h"
#include "ef/fn_80105314.h"
#include "ef/fn_8010D1A8.h"
#include "ef/cp_vector.h"
#include "ef/get_move_work_adrs.h"
#include "enemy/em009_act.h"
#include "lobby/fn_8030121C.h" /* eft_em_spawn, C linkage (rule 2: the owner's header) */
#include "mh3_pad.h"
#include "mh3_pad/system_w.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "stage/shell_set_func_ptr.h" /* `shell_set_func_ptr` and its slots (rule 2: the owner's header) */
#include "MSL/new.h" /* the placement `operator new` */
#include "enemy/fn_8011D448.h" /* em_parts_damage_level_get */
#include "enemy/fn_80138074.h"
#include "enemy/em_se_record.h"
#include "enemy/em_curve_key.h"
#include "enemy/em_common.h"
#include "draw_shape.h"
#include "stage/stg_w.h"
#include "enemy/em_roll_cooldown_ck.h"

#pragma peephole off
#pragma pool_data off
#pragma fp_contract off

/* Call-site views: each macro casts a callee to the signature its call sites use (the same direct call). */
#define draw_shape_arm_c1 ((void (*)(struct _ENEMY_WORK*, u32, u32))draw_shape_arm)
#define em_area_break_ck_c2 ((u8 (*)(struct _ENEMY_WORK*, u32))em_area_break_ck)
#define em_work_die_ck_c1 ((u32 (*)(struct _ENEMY_WORK*))em_work_die_ck)
#define em_pack_area_pick_c1 ((u32 (*)(struct _ENEMY_WORK*, u32, u8, u16, u8*, u32*))em_pack_area_pick)
#define em_roll_cooldown_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u16, u16, s16*))em_roll_cooldown_ck)
#define get_move_work_max_c1 ((u16 (*)(u8))get_move_work_max)
#define em_fall_start_c1 ((void (*)(struct _ENEMY_WORK*, f32))em_fall_start)
#define em_pl_target_ck_c1 ((u32 (*)(struct _ENEMY_WORK*, u8*))em_pl_target_ck)

/* The shell-set slots this unit calls, typed as the call sites pass them (the table register is only the
 * slot base, not an argument). */
typedef void (*EmShellPosAngleFn)(_ENEMY_WORK* self, nw4r::math::VEC3* pos, u32* angle, u32 kind, u32 area,
                                  u16 flags);
typedef void (*EmShellJobFn)(_ENEMY_WORK* self, u32 kind, u32 count, nw4r::math::VEC3* pos, f32 scale, u16 id);

/* The user-data accessor `em009_init` installs: the base accessor with a destructor of its own only (its table,
 * 0x805F0C88, keeps every hook of `ResUserDataAc`).  size: 0xC (`operator new(0xC)`) */
class Em009UserData : public ResUserDataAc {
public:
    Em009UserData();
    virtual ~Em009UserData();
};

/* The em009 program table (`em009_prog_tbl`): the entry points the shared enemy program driver calls (slot names
 * follow `Em008ProgTbl`'s scheme, a GUESS from the functions they hold).
 * size: 0x70 */
struct Em009ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(_ENEMY_WORK* self);
    /* +0x04 */ void (*init_0x04)(_ENEMY_WORK* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(_ENEMY_WORK* self);
    /* +0x0C */ void (*action_run_0x0C)(_ENEMY_WORK* self);
    /* +0x10 */ void (*motion_next_0x10)(_ENEMY_WORK* self, u8* state, u8* sub);
    /* +0x14 */ void (*event_notify_0x14)(_ENEMY_WORK* self, u8 kind, u8 sub);
    /* +0x18 */ void (*motion_events_0x18)(_ENEMY_WORK* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tev_color_0x20)(void);
    /* +0x24 */ u32 (*state_query_0x24)(_ENEMY_WORK* self, u8 kind);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*death_action_0x30)(_ENEMY_WORK* self, u8* action, u8* sub);
    /* +0x34 */ void (*cooldown_clear_0x34)(_ENEMY_WORK* self);
    /* +0x38 */ u32 pad_0x38;
    /* +0x3C */ int (*part_broken_ck_0x3C)(_ENEMY_WORK* self);
    /* +0x40 */ u32 pad_0x40[5];
    /* +0x54 */ u32 (*action_select_0x54)(_ENEMY_WORK* self, u32 arg);
    /* +0x58 */ u32 pad_0x58[6];
};

extern "C" Em009ProgTbl em009_prog_tbl = {
    em009_work_clear,
    em009_init,
    em009_frame_update,
    em009_action_dispatch,
    em009_motion_next,
    em009_event_notify,
    em009_motion_events,
    0,
    em009_tev_color_update,
    em009_state_query,
    {0, 0},
    em009_death_action_get,
    em009_cooldown_clear,
    0,
    em009_part_broken_ck,
    {0, 0, 0, 0, 0},
    em009_action_select,
    {0, 0, 0, 0, 0, 0},
};


EmCurveKey em009_slide_curve[23] = {
    {0.0f, -9.0f},
    {55.9f, -9.0f},
    {56.0f, 1.5f},
    {99.9f, 1.5f},
    {100.0f, 11.0f},
    {133.9f, 11.0f},
    {134.0f, 3.0f},
    {175.9f, 3.0f},
    {176.0f, -2.0f},
    {195.9f, -2.0f},
    {196.0f, 11.0f},
    {227.9f, 11.0f},
    {228.0f, 1.0f},
    {251.9f, 1.0f},
    {252.0f, 3.0f},
    {285.9f, 3.0f},
    {286.0f, -2.0f},
    {337.9f, -2.0f},
    {338.0f, 5.0f},
    {369.9f, 5.0f},
    {370.0f, 0.0f},
    {416.0f, 0.0f},
    {-1.0f, 0.0f},
};

u32 em009_pick_tbl[3] = {0x14, 0x00C8000A, 0xFFFF0000};

extern "C" const u32 em009_turn_seq_0[16] = {
    0x00000008, 0x0000002A, 0x00000000, 0x40001555,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x00080000, 0x00000004, 0x00000000, 0x00000000,
    0x00070000, 0x00000004, 0x00000000, 0x00000000,
};

extern "C" const u32 em009_turn_seq_1[16] = {
    0x00000008, 0x0000002A, 0x00000000, 0x80000000,
    0x00200000, 0x00000004, 0x00000000, 0x00000000,
    0x000D0000, 0x00000002, 0x00000000, 0x00000000,
    0x000C0000, 0x00000002, 0x00000000, 0x00000000,
};

extern "C" {

/* 0x803868DC (0x14): Clears the joint-effect handle and the two +0x338/+0x33A counters. */
void em009_work_clear(_ENEMY_WORK* self) {
    self->handles_0x328[0] = 0;
    self->init_0x328.field_0x338 = 0;
    self->init_0x328.field_0x33A = 0;
}

/* 0x803868F0 (0xD8): The init slot: hands the motion over on mode 2, attaches the user-data helper when there is
 * none and arms the position effect outside the +0x009 gate. */
void em009_init(_ENEMY_WORK* self, u8 mode) {
    nw4r::math::VEC3 v;
    Em009UserData* helper;

    VEC3_ctor(&v);
    switch (mode) {
    case 2:
        em_move_mode_set(self, 4);
        em_act_arm_unless_down(self, 6, 5);
        em_state_refresh(self);
        break;
    }
    if (em_res_user_data_ck(self) == 0) {
        helper = (Em009UserData*)operator new(sizeof(Em009UserData));
        new (helper) Em009UserData;
        em_res_user_data_set(self, helper);
    }
    if (self->field_0x009 == 0) {
        setVector3(&v, 0.0f, 40.0f, 80.0f);
        eft_em_attach_set(self, 0x15, &v, 2.0f, 0x1C72);
    }
}

} /* extern "C" */

/* 0x803869C8 (0x3C): Constructs the accessor: the base first, then this class's table. */
Em009UserData::Em009UserData() {
}

extern "C" {

/* 0x80386A04 (0x298): In state 7, moves the sub-state on to its follow-up once the effect runs (sub-state 1/0x27 to
 * 0x43 while part flag bit 0 is set). */
void em009_motion_next(_ENEMY_WORK* self, u8* state, u8* sub) {
    switch (*state) {
    case 7:
        switch (*sub) {
        case 0x1:
        case 0x27:
            if ((self->flags_0x836 & 1) != 0) {
                *sub = 0x43;
            }
            break;
        case 0x6:
            if (em_eft_active_ck(self) == 1) {
                *sub = 7;
            }
            break;
        case 0xD:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0xF;
            }
            break;
        case 0xE:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x10;
            }
            break;
        case 0x11:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x13;
            }
            break;
        case 0x12:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x14;
            }
            break;
        case 0x15:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x17;
            }
            break;
        case 0x16:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x18;
            }
            break;
        case 0x19:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x1B;
            }
            break;
        case 0x1A:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x1C;
            }
            break;
        case 0x22:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x23;
            }
            break;
        case 0x2B:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x2C;
            }
            break;
        case 0x2D:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x2E;
            }
            break;
        case 0x2F:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x30;
            }
            break;
        case 0x31:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x32;
            }
            break;
        case 0x33:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x34;
            }
            break;
        case 0x35:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x36;
            }
            break;
        case 0x37:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x38;
            }
            break;
        case 0x39:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x3A;
            }
            break;
        case 0x3B:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x3C;
            }
            break;
        case 0x3D:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x3E;
            }
            break;
        case 0x3F:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x40;
            }
            break;
        case 0x41:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x42;
            }
            break;
        case 0x44:
            if (em_eft_active_ck(self) == 1) {
                *sub = 0x45;
            }
            break;
        }
        break;
    }
}

/* 0x80386C9C (0x600): The event hook: action 7's sub-states 0x24..0x2A request one to three spawns at joint 0x2A,
 * each turned by its own angle; action 1 and 10 raise the act flag and the part-0 hit. */
void em009_event_notify(_ENEMY_WORK* self, u8 kind, u8 sub) {
    nw4r::math::VEC3 pos;
    _CP_VECTOR rot;

    VEC3_ctor(&pos);
    switch (kind) {
    case 1:
        switch (sub) {
        case 8:
            em_act_flag1_set(self);
            break;
        }
        break;
    case 7:
        switch (sub) {
        case 0x24:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 0;
                rot.z = 0;
                rot.y = (u16)(self->field_0x1C0 + 0x8000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
            }
            break;
        case 0x25:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 0;
                rot.z = 0;
                rot.y = (u16)(self->field_0x1C0 + 0x8000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 0;
                rot.z = 1;
                rot.y = (u16)(self->field_0x1C0 + 0x9000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
            }
            break;
        case 0x26:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 0;
                rot.z = 0;
                rot.y = (u16)(self->field_0x1C0 + 0x7000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 0;
                rot.z = 1;
                rot.y = (u16)(self->field_0x1C0 + 0x8000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 0;
                rot.z = 1;
                rot.y = (u16)(self->field_0x1C0 + 0x9000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
            }
            break;
        case 0x28:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 1;
                rot.z = 0;
                rot.y = (u16)(self->field_0x1C0 + 0x8000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
            }
            break;
        case 0x29:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 1;
                rot.z = 0;
                rot.y = (u16)(self->field_0x1C0 + 0x8000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 1;
                rot.z = 1;
                rot.y = (u16)(self->field_0x1C0 + 0xA000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
            }
            break;
        case 0x2A:
            if (em_busy_ck(self) == 1) {
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 1;
                rot.z = 0;
                rot.y = (u16)(self->field_0x1C0 + 0xA000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 1;
                rot.z = 1;
                rot.y = (u16)(self->field_0x1C0 + 0x8000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
                get_joint_wpos_em(self, 0x2A, &pos);
                rot.x = 1;
                rot.z = 1;
                rot.y = (u16)(self->field_0x1C0 + 0x6000);
                em_spawn_request(self->field_0x01A, 0x28, 8, self->area_no, 0xFF, 1, 0xFF, 1, 0xFF, &pos, (s32)&rot);
            }
            break;
        }
        break;
    case 10:
        switch (sub) {
        case 0xC9:
            em_part_hit_set(self, 0, 0);
            break;
        }
        break;
    }
}

/* 0x8038729C (0x2C): Counts the +0x338 counter up to 1800 and the +0x33A cooldown down to zero. */
void em009_frame_update(_ENEMY_WORK* self) {
    if (self->init_0x328.field_0x338 < 1800) {
        self->init_0x328.field_0x338++;
    }
    if (self->init_0x328.field_0x33A > 0) {
        self->init_0x328.field_0x33A--;
    }
}

/* 0x803872C8 (0x7C): Plays motion 1 once, then finishes the action. */
void em009_act0_0(_ENEMY_WORK* self) {
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

/* 0x80387344 (0x7C): Plays motion 2 once, then finishes the action. */
void em009_act0_1(_ENEMY_WORK* self) {
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

/* 0x803873C0 (0x7C): Plays motion 0xE once, then finishes the action. */
void em009_act0_2(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 14, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8038743C (0x8C): Stands up in move mode 4 at the base height, then finishes standing. */
void em009_act0_7(_ENEMY_WORK* self) {
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

/* 0x803874C8 (0x60): Action 0's dispatcher by `state_sub`. */
void em009_act0_dispatch(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em009_act0_0(self);
        break;
    case 1:
        em009_act0_1(self);
        break;
    case 2:
        em009_act0_2(self);
        break;
    case 3:
        em009_act0_0(self);
        break;
    case 4:
        em009_act0_0(self);
        break;
    case 6:
        em009_act0_0(self);
        break;
    case 7:
        em009_act0_7(self);
        break;
    }
}

/* 0x80387528 (0x7C): Plays motion 0xF once, then finishes the action. */
void em009_act1_0(_ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x803875A4 (0x7C): Plays motion 9 once, then finishes the action. */
void em009_act1_1(_ENEMY_WORK* self) {
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

/* 0x80387620 (0x224): Motion 0xF with its two hit windows, the shape arm and the breath effects. */
void em009_act1_2(_ENEMY_WORK* self) {
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xF, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0xB, 5);
            draw_shape_arm_c1(self, 0x15, 0xA);
        }
        if (em_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 0, 0x14, &v, 1.0f);
        }
        if (em_frame_check(self, 3, 100.0f, 180.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            setVector3(&v, 0.0f, 0.0f, 100.0f);
            eft_em_spawn(self, 1, 0x14, &v, 1.0f);
        }
        if (em_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x16, 0xA);
        }
        if (em_frame_check(self, 0, 188.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 232.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x16, 0x1A);
        }
        if (em_frame_check(self, 0, 242.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x80387844 (0x1A8): The long tint hold: 1800 frames, or until a player in range is targeted, then the reaction
 * request. */
void em009_act1_3(_ENEMY_WORK* self)
{
    u32 count;
    u8* move;
    u16 i;
    u32 found;

    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x28, 2, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x6E, 4, 0);
            self->timer_0x020 = 0x708;
            em_tint_hold_start(self);
        }
        return;
    case 2:
        em_gauge_tick(self, 0.6f, 1, 0xF);
        if (--self->timer_0x020 <= 0) {
            found = 1;
        } else if (em_busy_ck(self) == 1) {
            count = get_move_work_max(2);
            move = (u8*)get_move_work_adrs(2);
            found = 0;
            for (i = 0; i < (u16)count; i++, move += 0xB20) {
                if (move[0] != 0 && em_pl_slot_leaving_ck(move[8]) == 0 && em_pl_target_ck_c1(self, move) == 1) {
                    found = 1;
                }
            }
        }
        if (found == 1) {
            em_reaction_req(self, 5, 8, 4);
            em_state_set(self, 1, 6);
        }
        return;
    }
}

/* 0x803879EC (0x23C): Motion 0xC8: the area effect, the breath spawn every eighth frame and the footfall bursts,
 * then state 5. */
void em009_act1_4(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xC8, 4, 0);
        return;
    case 1:
        if (em_frame_check(self, 0, 150.0f, 0.0f) == 1) {
            eft_spawn_type_at_area(self, 0x10);
        }
        if (em_frame_check(self, 3, 230.0f, 310.0f) == 1
            && (system_w.field_0x0c & 7) == 0) {
            setVector3(&v, 0.0f, -20.0f, 60.0f);
            eft_em_spawn(self, 0x90, 0x15, &v, 1.0f);
        }
        if (em_after_frame_check(self, 0, 360.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 372.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 384.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 396.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 408.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 420.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 432.0f, 0.0f) == 1
            || em_after_frame_check(self, 0, 444.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, -20.0f, 60.0f);
            eft_em_spawn(self, 0x8F, 0x15, &v, 1.0f);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_state_set(self, 1, 5);
        }
        return;
    }
}

/* 0x80387C28 (0x7C): Motion 0xCA with a stamina refill, then finishes the action. */
void em009_act1_5(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set(self, 0xCA, 4, 0);
        em_stamina_add(self, 0x3E8);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80387CA4 (0x350): The stun hold: the tint hold for 150 frames, the recovery motion, the turn back, the approach
 * and the landing, then the reaction request; the busy timer is held through every state. */
void em009_act1_6(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_mot_set_ck(self, 0x6E, 0, 0);
        self->timer_0x020 = 0x96;
        em_tint_hold_start(self);
        em_busy_set(self);
        em_busy_timer_reset(self);
        break;
    case 1:
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_gauge_tick(self, 0.6f, 1, 3);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            em_mot_set(self, 0x70, 4, 0);
            em_tint_hold_end(self);
        }
        break;
    case 2:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x26, 4, 0);
        }
        break;
    case 3:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_frame_check(self, 1, 200.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_move_mode_set(self, 4);
            em_turn_seq_start(self, (void*)em009_turn_seq_0, 0, 0, 0);
            em_height_set(self, em_height_base_get(self));
        }
        break;
    case 4:
        em_busy_set(self);
        em_busy_timer_reset(self);
        if (em_turn_seq_step(self, (void*)em009_turn_seq_0) == 1) {
            self->state++;
            em_move_mode_set(self, 4);
            em_mot_set(self, 0xA, 0, 0);
            em_approach_start(self, 0.0f, 0);
            em_height_set(self, em_height_base_get(self));
        }
        break;
    case 5:
        em_busy_set(self);
        em_busy_timer_reset(self);
        em_frame_flag_set(self);
        if (em_approach_step(self, 0, 0x100) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x27, 0, 0);
            em_height_set(self, 0.0f);
            em_shake_req_set(self);
        }
        break;
    case 6:
        if (em_frame_check(self, 2, 210.0f, 0.0f) == 1) {
            em_busy_set(self);
            em_busy_timer_reset(self);
        }
        if (em_frame_check(self, 2, 10.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set_ck(self, 1, 6, 0);
        }
        break;
    case 7:
        if (em_mot_frame_started_ck(self) == 1) {
            em_reaction_req(self, 0, 0, 0);
            em_state_set(self, 1, 7);
        }
        break;
    }
}

/* 0x80387FF4 (0xD4): Turns back along the turn table, plays motion 2, then finishes the action. */
void em009_act1_7(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em009_turn_seq_0, 0, 1, 0);
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em009_turn_seq_0) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set_ck(self, 2, 6, 0);
            return;
        }
        /* fallthrough */
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x803880C8 (0x7C): Plays motion 0xE once, then finishes the action. */
void em009_act1_8(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xE, 6, 0);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80388144 (0x4C): Action 1's dispatcher by `state_sub`. */
void em009_act1_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: em009_act1_0(self); return;
    case 1: em009_act1_1(self); return;
    case 2: em009_act1_2(self); return;
    case 3: em009_act1_3(self); return;
    case 4: em009_act1_4(self); return;
    case 5: em009_act1_5(self); return;
    case 6: em009_act1_6(self); return;
    case 7: em009_act1_7(self); return;
    case 8: em009_act1_8(self); return;
    }
}

/* 0x80388190 (0x1AC): The walk forward by `a`'s distance, then motion 0xB or the finish by `b`. */
void em009_act2_0(_ENEMY_WORK* self, u8 a, u8 b)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0xA, 4, 0);
        em_hit_window_set(self, 0, 0xC, 2);
        switch (a) {
        case 0: em_approach_start(self, 0.0f, 0); return;
        case 1: em_approach_start(self, -600.0f, 0); return;
        case 2: em_approach_start(self, -1000.0f, 0); return;
        case 3: em_approach_start(self, -2000.0f, 0); return;
        case 4: em_approach_start(self, 0.0f, 0); break;
        }
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1 && em_mot_frame_started_ck(self) == 0) {
            switch (b) {
            case 0:
                self->state++;
                em_mot_set(self, 0xB, 6, 0);
                return;
            case 1:
                em_action_finish(self);
                break;
            }
        }
        return;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038833C (0xB4): Turns along the turn table (`a` 1 waits for the motion first), then finishes the action. */
void em009_act2_6(_ENEMY_WORK* self, u8 a)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em009_turn_seq_0, 0, 1, 0);
        return;
    case 1:
        if (a == 1 && em_mot_end_ck(self) == 1) {
            em_action_finish(self);
            return;
        }
        if (em_turn_seq_step(self, (void*)em009_turn_seq_0) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x803883F0 (0xD8): Motion 0x21 turning to the target in its first frames, then finishes the action. */
void em009_act2_7(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x21, 4, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 58.0f) == 1) {
            em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * (2.0689654f * get_em_base_scale(self))) / 360.0f));
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x803884C8 (0x94): Motion 0x10 walking to the target point, then finishes the action. */
void em009_act2_9(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x10, 0xA, 0);
        em_approach_start(self, 0.0f, 0);
        return;
    case 1:
        if (em_approach_step(self, 0, 0x80) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038855C (0x128): The step back: the second turn table with the offset rotated toward the target point, the
 * fade cancel, then the finish. */
void em009_act2_10(_ENEMY_WORK* self)
{
    u32 ang;

    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_turn_seq_start(self, (void*)em009_turn_seq_1, 0, 1, 0);
        em_move_vec2_clr(self);
        self->field_0x318 = -6.481132f * get_em_base_scale(self) * get_em_chg_scale(self);
        self->field_0x324 = 0.0f;
        ang = calcVecAng2(&self->pos, &self->vec_0x36C);
        rotVecY(&self->offset_0x30C.vec_0x310, ang);
        rotVecY(&self->vec_0x31C, ang);
        return;
    case 1:
        if (em_frame_check(self, 3, 0.0f, 106.0f) == 1) {
            CancelFade(self);
        }
        if (em_turn_seq_step(self, (void*)em009_turn_seq_1) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80388684 (0xC8): Motion 0x1E (`a` 1) or 0x1F turning to the target, then finishes the action. */
void em009_act2_11(_ENEMY_WORK* self, u8 a)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, (u16)(a == 1 ? 0x1E : 0x1F), 2, 0);
        return;
    case 1:
        if (em_frame_check(self, 3, 14.0f, 78.0f) == 1) {
            em_turn_to_target(self, 0x100);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038874C (0x44C): The slide: motion 0x17 with the lava/ground trail and the offset push, then the stop motion.
 * */
void em009_act2_13(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        em_approach_start(self, 0.0f, 0x10);
        em_move_vec2_clr(self);
        self->offset_0x30C.vec_0x310.x = 0.0f;
        self->field_0x314 = 0.0f;
        self->field_0x318 = 23.6f * get_em_base_scale(self) * get_em_chg_scale(self);
        self->vec_0x31C.x = 0.0f;
        self->field_0x320 = 0.0f;
        self->field_0x324 = get_em_base_scale(self) * get_em_chg_scale(self);
        return;
    case 1:
        if (em_frame_check(self, 1, 168.0f, 0.0f) == 1
            && (system_w.field_0x0c & 3) == 0) {
            if (em_magma_check(self) == 1) {
                copyVec3(&v, &self->pos);
                v.y = 5.0f + self->field_0x214;
                eft009_set_pos(0x8B, &v, (struct _CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 150.0f);
                eft_em_spawn(self, 0x44, 1, &v, 1.0f);
                eft_em_spawn(self, 0x82, 1, &v, 1.0f);
            }
        }
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 6, 0xA);
        }
        if (em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
            em_move_offset_step(self, &self->field_0x1BC);
            if (self->field_0x318 > 50.0f) {
                self->field_0x318 = 50.0f;
            } else {
                em_mot_speed_set(self, 0.01f + self->field_0x7C4);
            }
        }
        if (em_approach_step(self, 0, 0x80) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0);
            self->field_0x318 *= 0.1f;
            self->vec_0x31C.x = 0.0f;
            self->field_0x320 = 0.0f;
            self->field_0x324 = -2.0f;
        }
        return;
    case 2:
        if (em_frame_check(self, 3, 40.0f, 52.0f) == 1
            && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x23, &v);
                v.y = self->field_0x214;
                eft009_set_pos(0x8C, &v, (struct _CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x45, 0x23, &v, 1.0f);
            }
        }
        if (em_frame_check(self, 3, 72.0f, 84.0f) == 1
            && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x23, &v);
                v.y = self->field_0x214;
                eft009_set_pos(0x8C, &v, (struct _CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x45, 0x23, &v, 1.0f);
            }
        }
        if (em_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->field_0x318 < 0.0f) {
            self->field_0x318 = 0.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80388B98 (0xA0): Action 2's dispatcher by `state_sub`. */
void em009_act2_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: em009_act2_0(self, 1, 0); return;
    case 1: em009_act2_0(self, 0, 1); return;
    case 2: em009_act2_0(self, 1, 1); return;
    case 3: em009_act2_0(self, 2, 1); return;
    case 4: em009_act2_0(self, 3, 1); return;
    case 5: em009_act2_0(self, 4, 1); return;
    case 6: em009_act2_6(self, 0); return;
    case 7: em009_act2_7(self); return;
    case 8: em009_act2_6(self, 1); return;
    case 9: em009_act2_9(self); return;
    case 10: em009_act2_10(self); return;
    case 11: em009_act2_11(self, 0); return;
    case 12: em009_act2_11(self, 1); return;
    case 13: em009_act2_13(self); return;
    }
}

/* 0x80388C38 (0xE4): Motion 0x26 with its two hit windows and the landing shake, then finishes standing. */
void em009_act6_0(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_hit_window_set_default(self, 0, 0x10);
        em_hit_window_set_default(self, 1, 0x11);
        return;
    case 1:
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

/* 0x80388D1C (0xAC): Stands up in move mode 4 for 90 frames, then finishes standing. */
void em009_act6_1(_ENEMY_WORK* self)
{
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
        if (--self->timer_0x020 == 0) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x80388DC8 (0xC0): Motion 6 backing off to the target point in move mode 4, then finishes standing. */
void em009_act6_2(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 6, 0, 0);
        em_approach_start(self, -200.0f, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x80388E88 (0x100): Motion 0xA approaching at triple speed (`a` picks the distance), then finishes standing. */
void em009_act6_3(_ENEMY_WORK* self, u8 a)
{
    f32 end = 0.0f;

    em_busy_set(self);
    em_busy_timer_reset(self);
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_mot_set(self, 0xA, 0, 0);
        em_mot_speed_set(self, 3.0f);
        if (a == 0) {
            end = -700.0f;
        }
        em_approach_start(self, end, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_approach_step(self, 0, 0x100) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x80388F88 (0xB0): Turns along the turn table in move mode 4, then finishes standing. */
void em009_act6_4(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 4);
        em_turn_seq_start(self, (void*)em009_turn_seq_0, 0, 0, 0);
        em_height_set(self, em_height_base_get(self));
        return;
    case 1:
        if (em_turn_seq_step(self, (void*)em009_turn_seq_0) == 1) {
            em_action_finish_stand(self);
        }
        return;
    }
}

/* 0x80389038 (0xE8): Motion 0x27 landing with its two hit windows and shakes, then finishes the action. */
void em009_act6_5(_ENEMY_WORK* self)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x27, 0, 0);
        em_height_set(self, 0.0f);
        em_hit_window_set_default(self, 0, 0x13);
        em_hit_window_set_default(self, 1, 0x14);
        em_shake_req_set(self);
        return;
    case 1:
        if (em_frame_check(self, 2, 10.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80389120 (0x514): The dive by `a`: the jump, the reaction request, the turn and the approach, the landing, then
 * the charge sub-state `a` selects. */
void em009_act6_10(_ENEMY_WORK* self, u8 a)
{
    em_busy_set(self);
    em_busy_timer_reset(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x26, 4, 0);
        em_hit_window_set_default(self, 0, 0x10);
        em_hit_window_set_default(self, 1, 0x11);
        return;
    case 1:
        if (em_frame_check(self, 1, 200.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            switch (a) {
            case 0:
            case 1:
            case 2:
                em_reaction_req(self, 5, 8, 7);
                break;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                em_reaction_req(self, 5, 8, 8);
                break;
            case 10:
            case 11:
            case 12:
                em_reaction_req(self, 5, 8, 9);
                break;
            }
            em_target_pos_set(self, 0);
            em_move_mode_set(self, 4);
            em_turn_seq_start(self, (void*)em009_turn_seq_0, 0, 0, 0);
            em_height_set(self, em_height_base_get(self));
        }
        return;
    case 2:
        if (em_turn_seq_step(self, (void*)em009_turn_seq_0) == 1) {
            self->state++;
            em_mot_set(self, 0xA, 0, 0);
            em_mot_speed_set(self, 3.0f);
            em_approach_start(self, 0.0f, 0);
            em_height_set(self, em_height_base_get(self));
            em_frame_flag_set(self);
        }
        return;
    case 3:
        em_frame_flag_set(self);
        if (em_approach_step(self, 0, 0x100) == 1) {
            if ((u32)(a - 3) > 6) {
                if (a <= 2 || (u32)(a - 0xA) <= 2) {
                    self->state++;
                    em_reaction_req(self, 5, 8, 2);
                    em_target_pos_set(self, 0);
                    em_turn_seq_start(self, (void*)em009_turn_seq_0, 0, 0, 0);
                    em_height_set(self, em_height_base_get(self));
                    return;
                }
            } else {
                self->state = 5;
                em_move_mode_set(self, 0);
                em_mot_set(self, 0x27, 0, 0);
                self->field_0x1BC = 0;
                self->field_0x1C0 = 0xDC00;
                self->field_0x1C4 = 0;
                em_height_set(self, 0.0f);
                em_hit_window_set_default(self, 0, 0x13);
                em_hit_window_set_default(self, 1, 0x14);
                em_shake_req_set(self);
                return;
            }
        }
        break;
    case 4:
        if (em_turn_seq_step(self, (void*)em009_turn_seq_0) == 1) {
            self->state++;
            em_move_mode_set(self, 0);
            em_mot_set(self, 0x27, 0, 0);
            em_height_set(self, 0.0f);
            em_hit_window_set_default(self, 0, 0x13);
            em_hit_window_set_default(self, 1, 0x14);
            em_shake_req_set(self);
        }
        return;
    case 5:
        if (em_frame_check(self, 2, 10.0f, 0.0f) == 1) {
            em_shake_req_set(self);
        }
        if (em_mot_end_ck(self) == 1) {
            switch (a) {
            case 0: em_state_set(self, 7, 0x22); return;
            case 1: em_state_set(self, 7, 0x2B); return;
            case 2: em_state_set(self, 7, 0x2D); return;
            case 3: em_state_set(self, 7, 0x2F); return;
            case 4: em_state_set(self, 7, 0x31); return;
            case 5: em_state_set(self, 7, 0x33); return;
            case 6: em_state_set(self, 7, 0x35); return;
            case 7: em_state_set(self, 7, 0x37); return;
            case 8: em_state_set(self, 7, 0x39); return;
            case 9: em_state_set(self, 7, 0x3B); return;
            case 10: em_state_set(self, 7, 0x3D); return;
            case 11: em_state_set(self, 7, 0x3F); return;
            case 12: em_state_set(self, 7, 0x41); return;
            }
        }
        return;
    }
}

/* 0x80389634 (0xB4): Action 6's dispatcher by `state_sub`. */
void em009_act6_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: em009_act6_0(self); return;
    case 1: em009_act6_1(self); return;
    case 2: em009_act6_2(self); return;
    case 3: em009_act6_3(self, 0); return;
    case 4: em009_act6_4(self); return;
    case 5: em009_act6_5(self); return;
    case 6: em009_act6_3(self, 1); return;
    case 10: em009_act6_10(self, 0); return;
    case 11: em009_act6_10(self, 1); return;
    case 12: em009_act6_10(self, 2); return;
    case 13: em009_act6_10(self, 3); return;
    case 14: em009_act6_10(self, 4); return;
    case 15: em009_act6_10(self, 5); return;
    case 16: em009_act6_10(self, 6); return;
    case 17: em009_act6_10(self, 7); return;
    case 18: em009_act6_10(self, 8); return;
    case 19: em009_act6_10(self, 9); return;
    case 20: em009_act6_10(self, 0xA); return;
    case 21: em009_act6_10(self, 0xB); return;
    case 22: em009_act6_10(self, 0xC); return;
    }
}

/* 0x803896E8 (0x274): The bite: motion 0x12 turning to the target, the shell spawn at joint 0x14 and the shell job
 * with the camera shake. */
void em009_act7_0(_ENEMY_WORK* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v;
    f32 f;
    f32 g;
    u32 ang;

    MTX34_ctor(&mtx);
    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x12, 4, 0);
        em_hit_window_set(self, 0, 1, 8);
        em_stamina_add(self, -0x1E);
        ang = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (ang >= 0x8000) {
            ang = 0x10000 - ang;
        }
        self->timer_0x020 = (s16)ang;
        if ((s16)ang > 0x3000) {
            self->timer_0x020 = 0x3000;
        }
        return;
    case 1:
        if (em_frame_check(self, 3, 50.0f, 100.0f) == 1) {
            f = 360.0f * (f32)self->timer_0x020;
            g = f / 65536.0f;
            em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * ((g / 50.0f) * get_em_base_scale(self))) / 360.0f));
        }
        if (em_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, 0.0f, -80.0f, 210.0f);
            mulVecMatAddTrans(&v, &mtx);
            ((EmShellPosAngleFn)shell_set_func_ptr->method_0x18)(self, &v, &self->field_0x1BC, 0x2F, self->area_no, self->field_0xAEA);
        }
        if (em_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            em_camera_req(self, 0x15, 6);
            setVector3(&v, 0.0f, -140.0f, 130.0f);
            ((EmShellJobFn)shell_set_func_ptr->method_0x2C)(self, 9, 0x15, &v, 1.6f, self->field_0xAEA);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038995C (0x2F4): The spin: motion 0x15 turning through five windows while the slide curve pushes it along the
 * start angle; `a` 0/1 pick the follow-up spin-off sub-state at frame 202, `a` 2 just finishes. */
void em009_act7_1(_ENEMY_WORK* self, u8 a)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x15, 4, 0);
        em_move_vec2_clr(self);
        self->angle_0x328 = self->field_0x1C0;
        em_hit_window_set(self, 0, 2, 8);
        break;
    case 1:
        if (em_frame_check(self, 2, 186.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 1.25f);
        } else {
            em_mot_speed_set(self, 1.0f);
        }
        em_turn_in_window(self, 120.0f, 174.0f, -0x3D28);
        em_turn_in_window(self, 174.0f, 190.0f, -0x2444);
        em_turn_in_window(self, 190.0f, 218.0f, -0x505B);
        em_turn_in_window(self, 218.0f, 248.0f, -0x11C7);
        em_turn_in_window(self, 248.0f, 320.0f, -0x2E39);
        self->offset_0x30C.vec_0x310.x = 0.0f;
        self->offset_0x30C.vec_0x310.y = 0.0f;
        self->offset_0x30C.vec_0x310.z = 0.45f * (get_em_chg_scale(self) * (em_key_curve_eval(self, em009_slide_curve) * get_em_base_scale(self)));
        rotVecY(&self->offset_0x30C.vec_0x310, self->angle_0x328);
        em_move_offset_apply(self);
        switch (a) {
        case 0:
        case 1:
            if (em_frame_check(self, 0, 202.0f, 0.0f) == 1) {
                if (a == 0) {
                    switch (self->bits_0x1EC % 3) {
                    case 0:
                        em_state_set(self, 7, 0x24);
                        break;
                    case 1:
                        em_state_set(self, 7, 0x25);
                        break;
                    case 2:
                        em_state_set(self, 7, 0x26);
                        break;
                    }
                } else {
                    switch (self->bits_0x1EC % 3) {
                    case 0:
                        em_state_set(self, 7, 0x28);
                        break;
                    case 1:
                        em_state_set(self, 7, 0x29);
                        break;
                    case 2:
                        em_state_set(self, 7, 0x2A);
                        break;
                    }
                }
            }
            break;
        case 2:
            if (em_mot_end_ck(self) == 1) {
                em_action_finish(self);
            }
            break;
        }
        break;
    }
}

/* 0x80389C50 (0x1CC): The spin-off: motion 0x15 turning through five windows with the slide curve push, then
 * finishes the action. */
void em009_act7_36(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 0x15, 0, 0);
        em_move_vec2_clr(self);
        if (self->field_0xAE9 == 1) {
            em_hit_window_set(self, 0, 2, 0x10);
        } else {
            em_hit_window_set_default(self, 0, 2);
        }
        /* fallthrough */
    case 1:
        if (em_frame_check(self, 2, 186.0f, 0.0f) == 1) {
            em_mot_speed_set(self, 1.25f);
        } else {
            em_mot_speed_set(self, 1.0f);
        }
        em_turn_in_window(self, 120.0f, 174.0f, -0x3D28);
        em_turn_in_window(self, 174.0f, 190.0f, -0x2444);
        em_turn_in_window(self, 190.0f, 218.0f, -0x505B);
        em_turn_in_window(self, 218.0f, 248.0f, -0x11C7);
        em_turn_in_window(self, 248.0f, 320.0f, -0x2E39);
        self->offset_0x30C.vec_0x310.x = 0.0f;
        self->offset_0x30C.vec_0x310.y = 0.0f;
        self->offset_0x30C.vec_0x310.z = 0.45f * (get_em_chg_scale(self) * (em_key_curve_eval(self, em009_slide_curve) * get_em_base_scale(self)));
        rotVecY(&self->offset_0x30C.vec_0x310, self->angle_0x328);
        em_move_offset_apply(self);
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x80389E1C (0x37C): The tail sweep (`a` 1 picks the lava variant): the sweep effects, the hit window and the
 * recovery motion. */
void em009_act7_2(_ENEMY_WORK* self, u8 a)
{
    nw4r::math::VEC3 v;
    u32 id1;
    u32 kind;
    u32 id2;

    VEC3_ctor(&v);
    switch (a) {
    default:
        id1 = 0x7F;
        id2 = 0x4C;
        kind = 3;
        break;
    case 1:
        id1 = 0xCB;
        id2 = 0xCA;
        kind = 0xF;
        break;
    }
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x13, 4, 0);
        em_mot_speed_set(self, 1.15f);
        em_stamina_add(self, -0xA);
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        return;
    case 1:
        if (em_after_frame_check(self, 3, 80.0f, 140.0f) == 1
            && (system_w.field_0x0c & 3) == 0) {
            setVector3(&v, 0.0f, 0.0f, 0.0f);
            eft_em_spawn(self, id1, 3, &v, 1.0f);
        }
        if (em_after_frame_check(self, 3, 198.0f, 262.0f) == 1
            && (system_w.field_0x0c & 7) == 0) {
            if (self->state_0x007 == 0) {
                self->state_0x007 = 1;
                setVector3(&v, 0.0f, -200.0f, 0.0f);
                eft_em_spawn_param(self, id2, 3, &v, 1.0f, 0);
            } else {
                self->state_0x007 = 0;
                setVector3(&v, 0.0f, -200.0f, 0.0f);
                eft_em_spawn_param(self, id2, 3, &v, 1.0f, 0x2000);
            }
        }
        if (em_frame_check(self, 0, 218.0f, 0.0f) == 1) {
            self->state++;
            em_hit_window_set(self, 0, kind, 2);
        }
        return;
    case 2:
        if (em_after_frame_check(self, 3, 198.0f, 262.0f) == 1
            && (system_w.field_0x0c & 7) == 0) {
            if (self->state_0x007 == 0) {
                self->state_0x007 = 1;
                setVector3(&v, 0.0f, -200.0f, 0.0f);
                eft_em_spawn_param(self, id2, 3, &v, 1.0f, 0);
            } else {
                self->state_0x007 = 0;
                setVector3(&v, 0.0f, -200.0f, 0.0f);
                eft_em_spawn_param(self, id2, 3, &v, 1.0f, 0x2000);
            }
        }
        if (em_mot_end_ck(self) == 1) {
            self->state_0x006++;
            if (a == 0 && self->state_0x006 == 3) {
                em_hit_window_set(self, 0, kind, 2);
            }
        }
        if (self->state_0x006 >= 4) {
            self->state++;
            em_mot_set(self, 0x14, 4, 0);
            em_hit_window_clear_all(self);
        }
        return;
    case 3:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038A198 (0x620): The triple bite: motion 0x16 with its four hit windows and shell spawns, turning to the
 * target and through the sweep windows. */
void em009_act7_3(_ENEMY_WORK* self, u8 a)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 v;
    f32 f;
    f32 g;
    u32 ang;

    MTX34_ctor(&mtx);
    VEC3_ctor(&v);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x16, 6, 0);
        em_mot_speed_set(self, 1.3f);
        em_stamina_add(self, -0xA);
        if ((u32)(a - 3) > 2) {
            switch (a) {
            default: self->state_0x007 = 0; break;
            case 1: case 6: self->state_0x007 = 1; break;
            case 2: case 7: self->state_0x007 = 2; break;
            }
        } else {
            self->state_0x007 = self->bits_0x1EC % 3;
        }
        ang = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (ang >= 0x8000) {
            ang = 0x10000 - ang;
        }
        self->timer_0x020 = (s16)ang;
        if ((s16)ang > 0x3000) {
            self->timer_0x020 = 0x3000;
        }
        return;
    case 1:
        if (em_frame_check(self, 0, 80.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 4, 0xA);
        }
        if (em_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 134.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 4, 0x1A);
        }
        if (em_frame_check(self, 0, 144.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 216.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 4, 0x1A);
        }
        if (em_frame_check(self, 0, 226.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 270.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 4, 0x1A);
        }
        if (em_frame_check(self, 0, 278.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        if (em_frame_check(self, 0, 88.0f, 0.0f) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, 0.0f, -80.0f, 210.0f);
            mulVecMatAddTrans(&v, &mtx);
            ((EmShellPosAngleFn)shell_set_func_ptr->method_0x18)(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA);
            em_camera_req(self, 0x15, 9);
        }
        if (em_frame_check(self, 0, 144.0f, 0.0f) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, 0.0f, -80.0f, 210.0f);
            mulVecMatAddTrans(&v, &mtx);
            ((EmShellPosAngleFn)shell_set_func_ptr->method_0x18)(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA);
            em_camera_req(self, 0x15, 9);
        }
        if (em_frame_check(self, 0, 226.0f, 0.0f) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, 0.0f, -80.0f, 210.0f);
            mulVecMatAddTrans(&v, &mtx);
            ((EmShellPosAngleFn)shell_set_func_ptr->method_0x18)(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA);
            em_camera_req(self, 0x15, 9);
        }
        if (em_frame_check(self, 0, 278.0f, 0.0f) == 1) {
            get_joint_wmat_em(self, 0x14, &mtx);
            setVector3(&v, 0.0f, -80.0f, 210.0f);
            mulVecMatAddTrans(&v, &mtx);
            ((EmShellPosAngleFn)shell_set_func_ptr->method_0x18)(self, &v, &self->field_0x1BC, 0x31, self->area_no, self->field_0xAEA);
            em_camera_req(self, 0x15, 9);
        }
        switch (a) {
        default:
            if (em_frame_check(self, 3, 48.0f, 76.0f) == 1) {
                f = 360.0f * (f32)self->timer_0x020;
                g = f / 65536.0f;
                em_turn_to_target(self, (u16)(s32)(0.5f + (65536.0f * ((g / 28.0f) * get_em_base_scale(self))) / 360.0f));
            }
            break;
        case 4: case 6:
            em_turn_in_window(self, 48.0f, 76.0f, 0x2E39);
            break;
        case 5: case 7:
            em_turn_in_window(self, 48.0f, 76.0f, -0x2E39);
            break;
        }
        switch (self->state_0x007) {
        case 1: em_turn_in_window(self, 178.0f, 208.0f, 0x2E39); break;
        case 2: em_turn_in_window(self, 178.0f, 208.0f, -0x2E39); break;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

}

u8 em009_route_points[13][12] = {
    {0x13, 0x16, 0x10, 0x11, 0x16, 0x17, 0x0E, 0x18, 0x0F, 0x15, 0x0B, 0xFF},
    {0x0E, 0x15, 0x0A, 0x0C, 0x0E, 0x17, 0x16, 0x11, 0x18, 0xFF, 0xFF, 0xFF},
    {0x10, 0x17, 0x0E, 0x07, 0x13, 0x16, 0x17, 0x0C, 0x0A, 0x15, 0x0F, 0xFF},
    {0x18, 0x11, 0x0F, 0x0E, 0x0C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x18, 0x11, 0x16, 0x17, 0x15, 0x0A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x18, 0x11, 0x10, 0x18, 0x0D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x0F, 0x0C, 0x0B, 0x15, 0x17, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x0F, 0x0C, 0x0D, 0x0E, 0x0F, 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x0F, 0x0C, 0x0B, 0x15, 0x18, 0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x18, 0x11, 0x16, 0x0F, 0x0C, 0x0B, 0x15, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x0F, 0x11, 0x16, 0x18, 0x0C, 0x0A, 0x15, 0x17, 0x10, 0xFF, 0xFF, 0xFF},
    {0x17, 0x15, 0x0B, 0x17, 0x16, 0x11, 0x07, 0x0E, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x18, 0x0B, 0x15, 0x0C, 0x12, 0x13, 0x16, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF},
};

extern "C" {

/* 0x8038A7B8 (0x10D0): The charge: run-up (by route `a`: a straight, a side-step, a curve, a circle or a patrol
 * over the area's data points), the slide with its lava/ground trail, and the stop (`b` picks the end motion). */
void em009_act7_5(_ENEMY_WORK* self, u8 a, u8 b) {
    nw4r::math::VEC3 v;
    u32 route;
    f32 lim;
    u32 done;
    f32 back;

    VEC3_ctor(&v);
    done = 0;
    lim = 95.0f;
    em_busy_set(self);
    route = a - 10;
    if (route <= 12) {
        em_busy_timer_reset(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->timer_0x020 = 0;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x17, 6, 0);
        em_mot_speed_set(self, 1.3f);
        em_hit_window_set(self, 0, 0x15, 8);
        em_stamina_add(self, -20);
        if (b == 1) {
            back = -1500.0f;
        } else {
            back = 0.0f;
        }
        switch (a) {
        default:
            em_approach_start(self, -500.0f + back, 0x10);
            break;
        case 2:
        case 3:
        case 4:
        case 5:
            em_approach_start(self, -300.0f + back, 0x10);
            break;
        case 6:
        case 7:
            em_approach_start(self, 500.0f, 0x10);
            break;
        case 8:
        case 9:
            em_approach_start(self, 0.0f, 0x10);
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
            em_frame_flag_set(self);
            em_fx_flag_set(self);
            self->init_0x328.field_0x338 = 0;
            em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | em009_route_points[route][self->state_0x006]),
                              &self->vec_0x36C);
            em_approach_start(self, 0.0f, 0x10);
            self->state_0x006++;
            break;
        case 23:
            em_approach_start(self, -800.0f + back, 0x10);
            break;
        }
        break;
    case 1:
        if ((u32)(a - 6) <= 1) {
            if (em_frame_check(self, 1, 30.0f, 0.0f) == 1) {
                em_mot_speed_set(self, 0.8f);
            }
            if (em_frame_check(self, 1, 92.0f, 0.0f) == 1) {
                em_mot_speed_set(self, 1.3f);
            }
        }
        switch (a) {
        default:
            if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                em_approach_step(self, 0, 0x100);
            } else {
                em_approach_step(self, 0, 0);
            }
            break;
        case 2:
            if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                self->field_0x1C0 += (u16)(s32)(0.5f + (65536.0f * (2.1538463f * (0.5f * get_em_base_scale(self)))) / 360.0f);
            }
            em_approach_step(self, 0, 0);
            break;
        case 3:
            if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                self->field_0x1C0 -= (u16)(s32)(0.5f + (65536.0f * (2.1538463f * (0.5f * get_em_base_scale(self)))) / 360.0f);
            }
            em_approach_step(self, 0, 0);
            break;
        case 4:
            if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                self->field_0x1C0 += (u16)(s32)(0.5f + (65536.0f * (2.1538463f * (0.8333333f * get_em_base_scale(self)))) / 360.0f);
            }
            em_approach_step(self, 0, 0);
            break;
        case 5:
            if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                self->field_0x1C0 -= (u16)(s32)(0.5f + (65536.0f * (2.1538463f * (0.8333333f * get_em_base_scale(self)))) / 360.0f);
            }
            em_approach_step(self, 0, 0);
            break;
        case 8:
            em_turn_in_window(self, 90.0f, 110.0f, -0x1555);
            em_approach_step(self, 0, 0);
            break;
        case 9:
            em_turn_in_window(self, 90.0f, 110.0f, 0x1555);
            em_approach_step(self, 0, 0);
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
            em_frame_flag_set(self);
            em_fx_flag_set(self);
            if (em_frame_check(self, 1, 90.0f, 0.0f) == 1) {
                em_approach_step(self, 0, 0x200);
            } else {
                em_approach_step(self, 0, 0);
            }
            break;
        }
        if (em_frame_check(self, 0, 120.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 6, 0x1B);
        }
        if (em_frame_check(self, 1, 164.0f, 0.0f) == 1) {
            self->state++;
            em_move_vec2_clr(self);
            self->offset_0x30C.vec_0x310.x = 0.0f;
            self->offset_0x30C.vec_0x310.y = 0.0f;
            self->offset_0x30C.vec_0x310.z = 23.6f * get_em_base_scale(self) * get_em_chg_scale(self);
            self->vec_0x31C.x = 0.0f;
            self->vec_0x31C.y = 0.0f;
            self->vec_0x31C.z = 4.0f * get_em_base_scale(self) * get_em_chg_scale(self);
            em_hit_window_set(self, 0, 0x18, 0x1B);
        }
        break;
    case 2:
        switch (a) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
            lim = 150.0f;
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            lim = 95.0f;
            break;
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z > lim) {
            self->offset_0x30C.vec_0x310.z = lim;
        } else {
            em_mot_speed_set(self, self->field_0x7C4 + 0.01f * get_em_base_scale(self));
        }
        if (em_frame_check(self, 1, 168.0f, 0.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            if (em_magma_check(self) == 1) {
                copyVec3(&v, &self->pos);
                v.y = self->field_0x214;
                eft009_set_pos(0x8B, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 150.0f);
                eft_em_spawn(self, 0x44, 1, &v, 1.0f);
                eft_em_spawn(self, 0x82, 1, &v, 1.0f);
            }
        }
        switch (a) {
        case 0:
            em_turn_to_target(self, 0x100);
            em_approach_step(self, 0, 0);
            break;
        default:
            if (em_approach_step(self, 0, 0x80) == 1 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                done = 1;
            }
            break;
        case 2:
            self->field_0x1C0 += 0x5B;
            if (em_approach_step(self, 0, 0) == 1 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                done = 1;
            }
            break;
        case 3:
            self->field_0x1C0 -= 0x5B;
            if (em_approach_step(self, 0, 0) == 1 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                done = 1;
            }
            break;
        case 4:
            self->field_0x1C0 += 0x98;
            if (em_approach_step(self, 0, 0) == 1 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                done = 1;
            }
            break;
        case 5:
            self->field_0x1C0 -= 0x98;
            if (em_approach_step(self, 0, 0) == 1 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                done = 1;
            }
            break;
        case 6:
        case 7:
            switch (self->state_0x006) {
            case 0:
                if (em_approach_step(self, 0, 0) == 1) {
                    self->state_0x006++;
                    self->angle_0x328 = 0;
                }
                break;
            case 1:
                self->timer_0x020++;
                if (a == 6) {
                    self->field_0x1C0 += (u16)(s32)(0.5f + (65536.0f * (6.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                } else {
                    self->field_0x1C0 -= (u16)(s32)(0.5f + (65536.0f * (6.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                }
                self->angle_0x328 += (u16)(s32)(0.5f + (65536.0f * (6.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                if (self->angle_0x328 >= 0xC000) {
                    self->state_0x006++;
                    self->timer_0x020 = 0;
                }
                break;
            case 2:
                if (++self->timer_0x020 >= 15 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                    done = 1;
                }
                break;
            }
            break;
        case 8:
        case 9:
            switch (self->state_0x006) {
            case 0:
                if (++self->timer_0x020 >= 0) {
                    self->state_0x006++;
                    self->timer_0x020 = 0;
                    self->angle_0x328 = 0;
                }
                break;
            case 1:
                self->timer_0x020++;
                if (a == 8) {
                    self->field_0x1C0 += (u16)(s32)(0.5f + (65536.0f * (4.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                } else {
                    self->field_0x1C0 -= (u16)(s32)(0.5f + (65536.0f * (4.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                }
                self->angle_0x328 += (u16)(s32)(0.5f + (65536.0f * (4.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                if (self->angle_0x328 >= 0x8000) {
                    self->state_0x006++;
                    self->timer_0x020 = 0;
                    self->angle_0x328 = 0;
                }
                break;
            case 2:
                self->timer_0x020++;
                if (a == 8) {
                    self->field_0x1C0 += (u16)(s32)(0.5f + (65536.0f * (6.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                } else {
                    self->field_0x1C0 -= (u16)(s32)(0.5f + (65536.0f * (6.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                }
                self->angle_0x328 += (u16)(s32)(0.5f + (65536.0f * (6.0f * (self->offset_0x30C.vec_0x310.z / lim))) / 360.0f);
                if (self->angle_0x328 >= 0xAAAB) {
                    self->state_0x006++;
                    self->timer_0x020 = 0;
                }
                break;
            case 3:
                if (++self->timer_0x020 >= 5 && em_frame_check(self, 1, 166.0f, 0.0f) == 1) {
                    done = 1;
                }
                break;
            }
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
            em_frame_flag_set(self);
            em_fx_flag_set(self);
            if (em_approach_step(self, 0, 0x400) == 1) {
                u8 point = em009_route_points[route][self->state_0x006];
                if (point != 0xFF) {
                    em_data_point_get(self, (u16)(((self->area_no & 0xF) << 8) | point), &self->vec_0x36C);
                    em_approach_start(self, 0.0f, 0x10);
                    self->state_0x006++;
                } else {
                    done = 1;
                }
            }
            break;
        }
        if (done != 0) {
            if (b == 0) {
                self->state = 3;
                em_mot_set(self, 0x18, 0, 0);
                em_hit_window_set(self, 1, 0xE, 0x18);
            } else {
                self->state = 4;
                em_mot_set(self, 0x19, 0, 0);
                em_hit_window_set(self, 0, 0x17, 0x18);
            }
            self->offset_0x30C.vec_0x310.z *= 0.1f;
            self->vec_0x31C.x = 0.0f;
            self->vec_0x31C.y = 0.0f;
            self->vec_0x31C.z = -2.0f;
        }
        break;
    case 3:
        if (em_frame_check(self, 3, 40.0f, 52.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x23, &v);
                v.y = self->field_0x214;
                eft009_set_pos(0x8C, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x45, 0x23, &v, 1.0f);
            }
        }
        if (em_frame_check(self, 3, 72.0f, 84.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x23, &v);
                v.y = self->field_0x214;
                eft009_set_pos(0x8C, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x45, 0x23, &v, 1.0f);
            }
        }
        if (em_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em_hit_window_set(self, 0, 0x12, 0x10);
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z < 0.0f) {
            self->offset_0x30C.vec_0x310.z = 0.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    case 4:
        if (em_frame_check(self, 0, 108.0f, 0.0f) == 1) {
            em_hit_window_clear(self, 0);
        }
        em_move_offset_step(self, &self->field_0x1BC);
        if (self->offset_0x30C.vec_0x310.z < 0.0f) {
            self->offset_0x30C.vec_0x310.z = 0.0f;
        }
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 0x71, 0, 0);
        }
        break;
    case 5:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* 0x8038B888 (0x8C): Motion 0x1A with its hit window, then finishes the action. */
void em009_act7_8(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 0x1A, 4, 0);
        em_hit_window_set_default(self, 0, 5);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038B914 (0x120): Motion 0x1D or 0x1C (by `a`) with two hit windows, turning a quarter in the first 50 frames,
 * then finishes the action. */
void em009_act7_9(_ENEMY_WORK* self, u8 a)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        if (a == 0) {
            em_mot_set(self, 0x1D, 4, 0);
            em_hit_window_set_default(self, 0, 7);
            em_hit_window_set_default(self, 1, 8);
            return;
        }
        em_mot_set(self, 0x1C, 4, 0);
        em_hit_window_set_default(self, 0, 9);
        em_hit_window_set_default(self, 1, 0xA);
        return;
    case 1:
        if (a == 0) {
            em_turn_in_window(self, 0.0f, 50.0f, 0x4000);
        } else {
            em_turn_in_window(self, 0.0f, 50.0f, -0x4000);
        }
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038BA34 (0x2F4): Action 7's dispatcher by `state_sub`. */
void em009_act7_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x0: em009_act7_0(self); return;
    case 0x1: em009_act7_1(self, 0); return;
    case 0x2: em009_act7_2(self, 0); return;
    case 0x3: em009_act7_3(self, 0); return;
    case 0x4: em009_act7_2(self, 1); return;
    case 0x5: em009_act7_5(self, 0, 0); return;
    case 0x6: em009_act7_5(self, 1, 0); return;
    case 0x7: em009_act7_5(self, 1, 1); return;
    case 0x8: em009_act7_8(self); return;
    case 0x9: em009_act7_9(self, 0); return;
    case 0xA: em009_act7_9(self, 1); return;
    case 0xB: em009_act7_3(self, 1); return;
    case 0xC: em009_act7_3(self, 2); return;
    case 0xD: em009_act7_5(self, 2, 0); return;
    case 0xE: em009_act7_5(self, 3, 0); return;
    case 0xF: em009_act7_5(self, 2, 1); return;
    case 0x10: em009_act7_5(self, 3, 1); return;
    case 0x11: em009_act7_5(self, 4, 0); return;
    case 0x12: em009_act7_5(self, 5, 0); return;
    case 0x13: em009_act7_5(self, 4, 1); return;
    case 0x14: em009_act7_5(self, 5, 1); return;
    case 0x15: em009_act7_5(self, 6, 0); return;
    case 0x16: em009_act7_5(self, 7, 0); return;
    case 0x17: em009_act7_5(self, 6, 1); return;
    case 0x18: em009_act7_5(self, 7, 1); return;
    case 0x19: em009_act7_5(self, 8, 0); return;
    case 0x1A: em009_act7_5(self, 9, 0); return;
    case 0x1B: em009_act7_5(self, 8, 1); return;
    case 0x1C: em009_act7_5(self, 9, 1); return;
    case 0x1D: em009_act7_3(self, 3); return;
    case 0x1E: em009_act7_3(self, 4); return;
    case 0x1F: em009_act7_3(self, 5); return;
    case 0x20: em009_act7_3(self, 6); return;
    case 0x21: em009_act7_3(self, 7); return;
    case 0x22: em009_act7_5(self, 0xA, 0); return;
    case 0x23: em009_act7_5(self, 0xA, 1); return;
    case 0x24: em009_act7_36(self); return;
    case 0x25: em009_act7_36(self); return;
    case 0x26: em009_act7_36(self); return;
    case 0x27: em009_act7_1(self, 1); return;
    case 0x28: em009_act7_36(self); return;
    case 0x29: em009_act7_36(self); return;
    case 0x2A: em009_act7_36(self); return;
    case 0x2B: em009_act7_5(self, 0xB, 0); return;
    case 0x2C: em009_act7_5(self, 0xB, 1); return;
    case 0x2D: em009_act7_5(self, 0xC, 0); return;
    case 0x2E: em009_act7_5(self, 0xC, 1); return;
    case 0x2F: em009_act7_5(self, 0xD, 0); return;
    case 0x30: em009_act7_5(self, 0xD, 1); return;
    case 0x31: em009_act7_5(self, 0xE, 0); return;
    case 0x32: em009_act7_5(self, 0xE, 1); return;
    case 0x33: em009_act7_5(self, 0xF, 0); return;
    case 0x34: em009_act7_5(self, 0xF, 1); return;
    case 0x35: em009_act7_5(self, 0x10, 0); return;
    case 0x36: em009_act7_5(self, 0x10, 1); return;
    case 0x37: em009_act7_5(self, 0x11, 0); return;
    case 0x38: em009_act7_5(self, 0x11, 1); return;
    case 0x39: em009_act7_5(self, 0x12, 0); return;
    case 0x3A: em009_act7_5(self, 0x12, 1); return;
    case 0x3B: em009_act7_5(self, 0x13, 0); return;
    case 0x3C: em009_act7_5(self, 0x13, 1); return;
    case 0x3D: em009_act7_5(self, 0x14, 0); return;
    case 0x3E: em009_act7_5(self, 0x14, 1); return;
    case 0x3F: em009_act7_5(self, 0x15, 0); return;
    case 0x40: em009_act7_5(self, 0x15, 1); return;
    case 0x41: em009_act7_5(self, 0x16, 0); return;
    case 0x42: em009_act7_5(self, 0x16, 1); return;
    case 0x43: em009_act7_1(self, 2); return;
    case 0x44: em009_act7_5(self, 0x17, 0); return;
    case 0x45: em009_act7_5(self, 0x17, 1); return;
    }
}

}

EmSeRecord em009_se_rec_17 = {100, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_17[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_17},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_18 = {101, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_18[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_18},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_19 = {107, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_19[3] = {
    {0x18, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x46)},
    {0x00, {0, 0, 0}, &em009_se_rec_19},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_1B_a = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em009_se_rec_1B_b = {121, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em009_se_rec_1B_c = {113, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_1B[5] = {
    {0x00, {0, 0, 0}, &em009_se_rec_1B_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em009_se_rec_1B_b},
    {0x00, {0, 0, 0}, &em009_se_rec_1B_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_1C_a = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em009_se_rec_1C_b = {120, 0, 2, 0, 0, {0, 4, 0, 0}, 0};

EmSeRecord em009_se_rec_1C_c = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_1C[5] = {
    {0x00, {0, 0, 0}, &em009_se_rec_1C_a},
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x78)},
    {0x00, {0, 0, 0}, &em009_se_rec_1C_b},
    {0x00, {0, 0, 0}, &em009_se_rec_1C_c},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_1D = {123, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_1D[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_1D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_7A = {109, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_7A[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_7A},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_7B = {110, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em009_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_9F = {112, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_9F[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_9F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_7C = {111, 0, 4, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em009_se_seq_7C[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_7C},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_8D = {15, 0, 14, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_8D},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_78 = {100, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_78[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_78},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_7E_a = {104, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em009_se_rec_7E_b = {120, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em009_se_rec_7E_c = {112, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_7E[4] = {
    {0x00, {0, 0, 0}, &em009_se_rec_7E_a},
    {0x02, {0, 0, 0}, &em009_se_rec_7E_b},
    {0x00, {0, 0, 0}, &em009_se_rec_7E_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_7F_a = {105, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em009_se_rec_7F_b = {121, 0, 2, 0, 0, {1, 0, 0, 0}, 0};

EmSeRecord em009_se_rec_7F_c = {113, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_7F[4] = {
    {0x00, {0, 0, 0}, &em009_se_rec_7F_a},
    {0x02, {0, 0, 0}, &em009_se_rec_7F_b},
    {0x00, {0, 0, 0}, &em009_se_rec_7F_c},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_8E = {15, 0, 6, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_8E[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_8E},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_A8 = {100, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_A8[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_A8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_B6 = {117, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_B6[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_B6},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_B7 = {118, 0, 2, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em009_se_seq_B7[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_B7},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_B8 = {124, 0, 2, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_B8[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_B8},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_B9 = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_B9[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_B9},
    {0xFD, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_BA = {119, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em009_se_seq_BA[2] = {
    {0x01, {0, 0, 0}, &em009_se_rec_BA},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_BB = {119, 0, 0, 0, 0, {1, 0, 0, 0}, 0};

EmSeEntry em009_se_seq_BB[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_BB},
    {0xFE, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_BF_a = {107, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em009_se_rec_BF_b = {111, 0, 6, 0, 0, {2, 0, 0, 0}, 0};

EmSeEntry em009_se_seq_BF[5] = {
    {0x06, {0, 0, 0}, 0},
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x3C)},
    {0x00, {0, 0, 0}, &em009_se_rec_BF_a},
    {0x00, {0, 0, 0}, &em009_se_rec_BF_b},
    {0xFE, {0, 0, 0}, 0},
};

EmSePosRec em009_se_pos_C9_a = {140.0f, 160.0f, -1, -16384};

EmSePosRec em009_se_pos_C9_b = {250.0f, 290.0f, -1, -16384};

EmSeRecord em009_se_rec_C9_a = {122, 0, 2, 0, 0, {0, 8, 0, 0}, 0};

EmSeRecord em009_se_rec_C9_b = {122, 0, 0, 200, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_C9[6] = {
    {0x05, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xC8)},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em009_se_pos_C9_a},
    {0x00, {0, 0, 0}, &em009_se_rec_C9_a},
    {0x0A, {0, 0, 0}, (const EmSeRecord*)&em009_se_pos_C9_b},
    {0x00, {0, 0, 0}, &em009_se_rec_C9_b},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_alt_00 = {108, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_alt_00[3] = {
    {0x00, {0, 0, 0}, &em009_se_rec_alt_00},
    {0x07, {0, 0, 0}, 0},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_alt_0F = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_alt_0F[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_alt_0F},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_alt_1C = {119, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_alt_1C[2] = {
    {0x00, {0, 0, 0}, &em009_se_rec_alt_1C},
    {0xFF, {0, 0, 0}, 0},
};

EmSeRecord em009_se_rec_alt_1A_a = {109, 0, 4, 0, 0, {0, 1, 0, 0}, 0};

EmSeRecord em009_se_rec_alt_1A_b = {110, 0, 0, 0, 0, {0, 1, 0, 0}, 0};

EmSeEntry em009_se_seq_alt_1A[3] = {
    {0x00, {0, 0, 0}, &em009_se_rec_alt_1A_a},
    {0x00, {0, 0, 0}, &em009_se_rec_alt_1A_b},
    {0xFF, {0, 0, 0}, 0},
};

extern "C" {

/* 0x8038BD28 (0x358): Plays the sound table of the current sub-state, or finishes the action. */
void em009_act10_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0x17: em_se_tbl_play(self, em009_se_seq_17, 0, 0x17); return;
    case 0x18: em_se_tbl_play(self, em009_se_seq_18, 0, 0x18); return;
    case 0x19: em_se_tbl_play(self, em009_se_seq_19, 0, 0x19); return;
    case 0x1A: em_se_tbl_play(self, em009_se_seq_18, 0, 0x1A); return;
    case 0x1B: em_se_tbl_play(self, em009_se_seq_1B, 0, 0x1B); return;
    case 0x1C: em_se_tbl_play(self, em009_se_seq_1C, 0, 0x1C); return;
    case 0x1D: em_se_tbl_play(self, em009_se_seq_1D, 0, 0x1D); return;
    case 0x7A: em_se_tbl_play(self, em009_se_seq_7A, 0, 0x7A); return;
    case 0x7B: em_se_tbl_play(self, em009_se_seq_7B, 0, 0x7B); return;
    case 0x9F: em_se_tbl_play(self, em009_se_seq_9F, 0, 0x9F); return;
    case 0xA0: em_se_tbl_play(self, em009_se_seq_9F, 0, 0xA0); return;
    case 0x7C: em_se_tbl_play(self, em009_se_seq_7C, 0, 0x7C); return;
    case 0x8D: em_se_tbl_play(self, em009_se_seq_8D, 0, 0x8D); return;
    case 0x78: em_se_tbl_play(self, em009_se_seq_78, 0, 0x78); return;
    case 0xA8: em_se_tbl_play(self, em009_se_seq_A8, 0, 0xA8); return;
    case 0x7E: em_se_tbl_play(self, em009_se_seq_7E, 0, 0x7E); return;
    case 0x7F: em_se_tbl_play(self, em009_se_seq_7F, 0, 0x7F); return;
    case 0x8E: em_se_tbl_play(self, em009_se_seq_8E, 0, 0x8E); return;
    case 0xB6: em_se_tbl_play(self, em009_se_seq_B6, 0, 0xB6); return;
    case 0xB7: em_se_tbl_play(self, em009_se_seq_B7, 0, 0xB7); return;
    case 0xB8: em_se_tbl_play(self, em009_se_seq_B8, 0, 0xB8); return;
    case 0xB9: em_se_tbl_play(self, em009_se_seq_B9, 0, 0xB9); return;
    case 0xBA: em_se_tbl_play(self, em009_se_seq_BA, 0, 0xBA); return;
    case 0xBB: em_se_tbl_play(self, em009_se_seq_BB, 0, 0xBB); return;
    case 0xBC: em_se_tbl_play(self, em009_se_seq_BB, 0, 0xBC); return;
    case 0xBF: em_se_tbl_play(self, em009_se_seq_BF, 0, 0xBF); return;
    case 0xC1: em_se_tbl_play(self, em009_se_seq_8D, 0, 0xC1); return;
    case 0xC9: em_se_tbl_play(self, em009_se_seq_C9, 0, 0xC9); return;
    default: em_action_finish(self); return;
    }
}

/* 0x8038C080 (0x8C): Plays the alternate sound table of the current sub-state. */
void em009_act11_se_play(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: em_se_tbl_play_alt(self, em009_se_seq_alt_00, 0, 0); return;
    case 15: em_se_tbl_play_alt(self, em009_se_seq_alt_0F, 0, 0xF); return;
    case 26: em_se_tbl_play_alt(self, em009_se_seq_alt_1A, 0, 0x1A); return;
    case 28: em_se_tbl_play_alt(self, em009_se_seq_alt_1C, 0, 0x1C); return;
    default: em_se_tbl_play_alt(self, em009_se_seq_alt_00, 0, 0); return;
    }
}

/* 0x8038C10C (0x4): Runs the motion 0x27 landing of action 6. */
void em009_act12_0(_ENEMY_WORK* self)
{
    em009_act6_5(self);
}

/* 0x8038C110 (0x14): Action 12's dispatcher: sub-state 0 only. */
void em009_act12_dispatch(_ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        em009_act12_0(self);
    }
}

}

f32 em009_demo_keys3[44] = {
    0.0f, 8113.22f, 11641.97f, 10278.79f,
    172.0f, 8112.61f, 11643.93f, 10277.97f,
    180.0f, 8067.05f, 11770.55f, 10216.0f,
    190.0f, 7923.98f, 12049.62f, 10020.9f,
    200.0f, 7710.84f, 12437.44f, 9729.19f,
    212.0f, 7401.25f, 12461.62f, 9303.06f,
    222.0f, 7130.62f, 12122.38f, 8927.42f,
    234.0f, 6831.77f, 11102.49f, 8506.65f,
    240.0f, 6706.78f, 10591.44f, 8326.96f,
    242.0f, 6670.21f, 10466.91f, 8273.56f,
    -1.0f, 6670.21f, 10466.91f, 8273.56f,
};

f32 em009_demo_keys_a[92] = {
    0.0f, -3580.67f, 5236.58f, 7931.61f,
    14.0f, -3556.45f, 4795.17f, 7802.83f,
    36.0f, -3451.58f, 4456.36f, 7588.88f,
    42.0f, -3411.38f, 4508.47f, 7482.3f,
    68.0f, -3161.11f, 5208.33f, 6818.08f,
    86.0f, -2916.39f, 5307.39f, 6168.61f,
    106.0f, -2606.61f, 4647.28f, 5346.44f,
    128.0f, -2229.81f, 3038.86f, 4346.44f,
    146.0f, -1968.87f, 1497.35f, 3653.88f,
    152.0f, -1894.38f, 1404.41f, 3456.19f,
    166.0f, -1720.83f, 1628.94f, 2995.61f,
    182.0f, -1528.71f, 1756.98f, 2485.7f,
    192.0f, -1416.16f, 1574.12f, 2192.53f,
    206.0f, -1263.93f, 933.69f, 1787.91f,
    218.0f, -1130.5f, 88.17f, 1350.28f,
    222.0f, -1089.92f, 88.11f, 1208.53f,
    228.0f, -1032.81f, 167.53f, 999.81f,
    242.0f, -902.52f, 101.22f, 524.41f,
    258.0f, -718.74f, 93.49f, -51.99f,
    284.0f, -201.52f, 89.55f, -846.64f,
    316.0f, 677.16f, 91.8f, -1565.41f,
    376.0f, 2577.21f, 99.02f, -2483.66f,
    -1.0f, 2577.21f, 99.02f, -2483.66f,
};

f32 em009_demo_keys_a_time[10] = {
    0.0f, -192.03f, 240.0f, -192.03f,
    360.0f, -230.28f, 376.0f, -230.28f,
    -1.0f, -230.28f,
};

f32 em009_demo_keys_b[36] = {
    0.0f, 12.93f, -224.16f, 7.0f,
    27.3f, -233.25f, 10.0f, 34.55f,
    -238.26f, 15.0f, 186.17f, -344.17f,
    19.0f, 299.93f, -423.64f, 20.0f,
    306.42f, -428.16f, 24.0f, 396.87f,
    -488.48f, 43.0f, 810.55f, -706.52f,
    46.0f, 857.1f, -723.97f, 56.0f,
    950.18f, -809.14f, 80.0f, 1166.13f,
    -1040.62f, -1.0f, 1166.13f, -1040.62f,
};

extern "C" {

/* 0x8038C124 (0x9E8): The scripted entrance: the demo fall, the two key tracks with their lava/ground trail and
 * shakes, then the stand. */
void em009_act13_0(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    self->timer_0x020++;
    em_frame_flag_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 1, 0, 0);
        em_demo_reset(self, 0);
        self->timer_0x020 = 0;
        return;
    case 1:
        if (em_demo_time_ck(0x8C) == 1) {
            self->state++;
            em_demo_enable(self);
            em_fall_start_c1(self, 0.0f);
            em_mot_set(self, 0x17, 0, 0xA4);
            em_demo_key3_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys3, (void*)0);
            em_demo_rot_set(self, 0.0f, -129.75f, 0.0f);
        }
        return;
    case 2:
        if (em_frame_check(self, 1, 168.0f, 0.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&v, 0.0f, 0.0f, 150.0f);
            eft_em_spawn(self, 0x44, 1, &v, 1.0f);
        }
        em_demo_key3_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys3, (void*)0);
        if (em_demo_time_ck(0xF2) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_demo_rot_set(self, 0.0f, -192.03f, 0.0f);
            em_demo_key_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys_a, (void*)em009_demo_keys_a_time, 7, 2);
        }
        return;
    case 3:
        if (em_frame_check(self, 1, 168.0f, 0.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&v, 0.0f, 0.0f, 150.0f);
            eft_em_spawn(self, 0x44, 1, &v, 1.0f);
        }
        if (em_demo_frame_get() == 0x8F) {
            eft009_spawn_at_joint(self, 1, 6, 0, 1.0f);
        }
        if (em_demo_frame_get() == 0xC1 || em_demo_frame_get() == 0xE5) {
            copyVec3(&v, &self->pos);
            v.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&v, self->area_no, 1, 0, 1.2f);
        }
        em_demo_key_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys_a, (void*)em009_demo_keys_a_time, 7, 2);
        if (em_demo_time_ck(0x1DE) == 1) {
            self->state++;
            em_mot_speed_set(self, 1.25f);
        }
        return;
    case 4:
        if (em_frame_check(self, 1, 168.0f, 0.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&v, 0.0f, 0.0f, 150.0f);
            eft_em_spawn(self, 0x44, 1, &v, 1.0f);
        }
        em_demo_key_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys_a, (void*)em009_demo_keys_a_time, 7, 2);
        if (em_demo_time_ck(0x26C) == 1) {
            self->state++;
            self->timer_0x020 = 0;
            em_mot_set(self, 0x17, 0, 0xD6);
            em_demo_pos_set(self, 12.93f, 45.0f, -224.16f);
            em_demo_key_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys_b, NULL, 5, 0);
            em_demo_rot_set(self, 0.0f, 125.25f, 0.0f);
        }
        return;
    case 5:
        if (em_frame_check(self, 3, 214.0f, 222.0f) == 1 && (em_demo_frame_get() & 3) == 0) {
            setVector3(&v, 0.0f, 0.0f, 150.0f);
            eft_em_spawn(self, 0x44, 1, &v, 1.0f);
        }
        em_demo_key_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys_b, NULL, 5, 0);
        if (em_demo_time_ck(0x280) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0xA, 0xA);
        }
        return;
    case 6:
        if (em_frame_check(self, 0, 40.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &v);
            v.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&v, self->area_no, 4, 0, 1.0f);
        }
        if (em_frame_check(self, 3, 40.0f, 52.0f) == 1 && (em_demo_frame_get() % 3) == 0) {
            eft_em_spawn(self, 0x45, 0x23, NULL, 1.0f);
        }
        if (em_frame_check(self, 3, 48.0f, 60.0f) == 1 && (em_demo_frame_get() % 3) == 1) {
            eft_em_spawn(self, 0x45, 0x1C, NULL, 1.0f);
        }
        if (em_frame_check(self, 3, 72.0f, 84.0f) == 1 && (em_demo_frame_get() % 3) == 0) {
            eft_em_spawn(self, 0x45, 0x23, NULL, 1.0f);
        }
        em_demo_key_apply(self, (s16)self->timer_0x020, (void*)em009_demo_keys_b, NULL, 5, 0);
        if (em_demo_time_ck(0x2BE) == 1) {
            self->state++;
            em_mot_set(self, 0x18, 0, 0x48);
            em_demo_pos_set(self, -300.4792f, 35.0f, -718.8161f);
            em_demo_rot_set(self, 0.0f, 126.25f, 0.0f);
        }
        return;
    case 7:
        if (em_frame_check(self, 3, 72.0f, 84.0f) == 1 && (em_demo_frame_get() % 3) == 0) {
            eft_em_spawn(self, 0x45, 0x23, NULL, 1.0f);
        }
        if (em_frame_check(self, 0, 72.0f, 0.0f) == 1) {
            get_joint_wpos_em(self, 3, &v);
            v.y = 5.0f + self->field_0x20C;
            eft_spawn_pos_in_area(&v, self->area_no, 4, 0, 1.0f);
        }
        if (em_demo_time_ck(0x34C) == 1) {
            self->state++;
            em_mot_set(self, 2, 6, 0);
            em_demo_rot_set(self, 0.0f, 126.25f, 0.0f);
        }
        return;
    case 8:
        if (em_demo_time_ck(0x428) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0);
            em_demo_pos_set(self, -1083.3802f, 35.0f, -828.7873f);
            em_demo_rot_set(self, 0.0f, 86.25f, 0.0f);
        }
        return;
    case 9:
        if (em_demo_time_ck(0x4B6) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0x6E);
            em_demo_pos_set(self, -1483.3368f, 45.0f, -866.4448f);
            em_demo_rot_set(self, 0.0f, 86.25f, 0.0f);
        }
        return;
    case 10:
        if (em_demo_time_ck(0x4F2) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0xA0);
            em_demo_pos_set(self, -1479.4661f, 45.0f, -854.7481f);
        }
        return;
    case 11:
        if (em_demo_time_ck(0x526) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0xA6);
            em_demo_pos_set(self, -1484.9506f, 45.0f, -855.1076f);
            em_demo_rot_set(self, 0.0f, 86.25f, 0.0f);
        }
        return;
    case 12:
        if (em_demo_time_ck(0x5CE) == 1) {
            self->state++;
            em_mot_set(self, 0x12, 0, 0x164);
            em_demo_pos_set(self, -1442.4564f, 45.0f, -793.2897f);
            em_demo_rot_set(self, 0.0f, 86.25f, 0.0f);
        }
        return;
    case 13:
        if (em_demo_time_ck(0x624) == 1) {
            self->state++;
            em_mot_set(self, 2, 6, 0);
            em_demo_pos_set(self, -1343.641f, 45.0f, -845.8662f);
            em_demo_rot_set(self, 0.0f, 86.25f, 0.0f);
        }
        return;
    }
}

/* 0x8038CB0C (0xA4): The entrance pose: motion 2 at the demo position, then finishes the action. */
void em009_act13_1(_ENEMY_WORK* self)
{
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 2, 0, 0);
        em_demo_pos_set(self, -1343.641f, 45.0f, -845.8662f);
        em_demo_rot_set(self, 0.0f, 86.25f, 0.0f);
        return;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        return;
    }
}

/* 0x8038CBB0 (0x24): Action 13's dispatcher by `state_sub`. */
void em009_act13_dispatch(_ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0: em009_act13_0(self); return;
    case 1: em009_act13_1(self); return;
    }
}

/* 0x8038CBD4 (0x4C): The action slot: runs the dispatcher of the current action. */
void em009_action_dispatch(_ENEMY_WORK* self)
{
    switch (self->action) {
    case 0: em009_act0_dispatch(self); return;
    case 1: em009_act1_dispatch(self); return;
    case 2: em009_act2_dispatch(self); return;
    case 6: em009_act6_dispatch(self); return;
    case 7: em009_act7_dispatch(self); return;
    case 10: em009_act10_se_play(self); return;
    case 11: em009_act11_se_play(self); return;
    case 12: em009_act12_dispatch(self); return;
    case 13: em009_act13_dispatch(self); return;
    }
}

/* 0x8038CC20 (0x84): Spawns the alternate-mode aura every 32nd frame while the alternate mode is on. */
void em009_alt_mode_effect(_ENEMY_WORK* self)
{
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    if (em_alt_mode_ck(self) == 1 && (system_w.field_0x0c & 0x1F) == 0) {
        setVector3(&v, 0.0f, 0.0f, 80.0f);
        eft_spawn_type10(self, 0x18, 0x14, &v, 1.0f);
    }
}

/* 0x8038CCA4 (0x400): The effect router: route 0 spawns the footfall effect (the lava variants first spawn their
 * splash), route 1 the area effect (lava: the splash only), route 2 the type-11 effect, at joint `c` or the
 * position. */
void em009_effect_spawn(_ENEMY_WORK* self, u8 a, u8 b, u32 c, s32 d, f32 scale)
{
    nw4r::math::VEC3 v;
    u8 kind;

    kind = b;
    VEC3_ctor(&v);
    switch (a) {
    case 0:
        if ((self->field_0x228 & 6) != 0) {
            switch (kind) {
            case 0:
                kind = 13;
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = 5.0f + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(15, &v, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, c, 15, d, scale);
                }
                break;
            case 2:
                kind = 12;
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = 5.0f + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(14, &v, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, c, 14, d, scale);
                }
                break;
            case 4:
                kind = 19;
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = 5.0f + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(15, &v, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, c, 15, d, scale);
                }
                break;
            case 1:
            case 3:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 36:
            case 37:
            case 41:
            case 42:
                return;
            }
        } else {
            switch (kind) {
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 38:
                return;
            }
            if (c == 0xFF) {
                v.x = self->pos.x;
                v.y = 5.0f + self->field_0x20C;
                v.z = self->pos.z;
            }
        }
        if (c == 0xFF) {
            eft009_set_pos(kind, &v, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
        } else {
            eft009_spawn_at_joint(self, c, kind, d, scale);
        }
        break;
    case 1:
        if ((self->field_0x228 & 6) != 0) {
            switch (kind) {
            case 0:
            case 5:
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = 5.0f + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(17, &v, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, c, 17, d, scale);
                }
                return;
            case 1:
                if (c == 0xFF) {
                    v.x = self->pos.x;
                    v.y = 5.0f + self->field_0x210;
                    v.z = self->pos.z;
                    eft009_set_pos(16, &v, (_CP_VECTOR*)&self->field_0x1BC, scale, self->area_no);
                } else {
                    eft009_spawn_at_joint(self, c, 16, d, scale);
                }
                return;
            }
        }
        if (c == 0xFF) {
            copyVec3(&v, &self->pos);
        } else {
            get_joint_wpos_em(self, c, &v);
        }
        v.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_pos_in_area(&v, self->area_no, kind, d, scale);
        break;
    case 2:
        if (c == 0xFF) {
            copyVec3(&v, &self->pos);
        } else {
            get_joint_wpos_em(self, c, &v);
        }
        v.y = self->field_0x20C;
        scale *= get_em_chg_scale(self);
        eft_spawn_type11(self, &v, kind, scale);
        break;
    }
}

/* 0x8038D0A4 (0x1844): The motion-event slot: by motion number and frame, the footfall and impact effects, the lava
 * trail and the camera shakes. */
void em009_motion_events(_ENEMY_WORK* self) {
    nw4r::math::VEC3 v;

    VEC3_ctor(&v);
    em009_alt_mode_effect(self);
    switch (em_get_mot_no(self)) {
    case 0x2:
        if (em_after_frame_check(self, 0, 166.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 2.0f);
        }
        break;
    case 0x7:
        if (self->field_0x1E2 == 0) {
            if (em_after_frame_check(self, 0, 74.0f, 0.0f) == 1) {
                em009_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
                em_camera_req(self, 0x23, 7);
            }
            if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
                em_camera_req(self, 0x1C, 7);
            }
        }
        break;
    case 0x8:
        if (self->field_0x1E2 == 0) {
            if (em_after_frame_check(self, 0, 76.0f, 0.0f) == 1) {
                em009_effect_spawn(self, 0, 2, 0x23, 0, 1.5f);
                em_camera_req(self, 0x23, 7);
            }
            if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
                em_camera_req(self, 0x1C, 7);
            }
        }
        break;
    case 0xA:
        if (self->field_0x1E2 == 0) {
            if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 100.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 192.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 280.0f, 0.0f) == 1) {
                em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
                em_camera_req(self, 0x1C, 0);
            }
            if (em_after_frame_check(self, 0, 55.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 144.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 234.0f, 0.0f) == 1) {
                em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
                em_camera_req(self, 0x23, 0);
            }
        }
        break;
    case 0xB:
        if (em_after_frame_check(self, 0, 22.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
            em_camera_req(self, 0x23, 0);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, 0x1C, 0);
        }
        break;
    case 0xC:
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em_camera_req(self, 0x23, 7);
        }
        break;
    case 0xD:
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            em_camera_req(self, 0x1C, 7);
        }
        break;
    case 0xF:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
        }
        if (em_after_frame_check(self, 0, 94.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 2.2f);
        }
        if (em_after_frame_check(self, 0, 186.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 242.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 200.0f);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0x14, 0x7C, 0, 1.0f, &v);
            } else {
                eft009_spawn_at_pos(self, 0x14, 0x7B, 0, 1.0f, &v);
            }
            eft009_spawn_at_pos(self, 0x14, 0x7D, 0, 1.0f, &v);
            em_camera_req(self, 0x14, 9);
        }
        break;
    case 0x10:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
            em_camera_req(self, 0x23, 7);
        }
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, 0x1C, 7);
        }
        break;
    case 0x12:
        if (em_after_frame_check(self, 0, 180.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 200.0f);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0x14, 0x7A, 0, 1.0f, &v);
            } else {
                eft009_spawn_at_pos(self, 0x14, 0x32, 0, 1.0f, &v);
            }
            eft009_spawn_at_pos(self, 0x14, 0x4D, 0, 1.0f, &v);
        }
        break;
    case 0x15:
        if (em_after_frame_check(self, 0, 190.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x16:
        if (em_after_frame_check(self, 0, 78.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
        }
        if (em_after_frame_check(self, 0, 88.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 144.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 226.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 278.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 200.0f);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0x14, 0x7C, 0, 1.0f, &v);
            } else {
                eft009_spawn_at_pos(self, 0x14, 0x7B, 0, 1.0f, &v);
            }
            eft009_spawn_at_pos(self, 0x14, 0x7D, 0, 1.0f, &v);
        }
        if (em_after_frame_check(self, 0, 214.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
        }
        break;
    case 0x17:
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 0);
        }
        if (em_after_frame_check(self, 1, 162.0f, 0.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            em_camera_req(self, -1, 0);
        }
        break;
    case 0x18:
        if (em_after_frame_check(self, 0, 2.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 5);
        }
        break;
    case 0x19:
        if (em_after_frame_check(self, 3, 2.0f, 30.0f) == 1 && (system_w.field_0x0c & 1) == 0) {
            if (em_magma_check(self) == 1) {
                copyVec3(&v, &self->pos);
                v.y = 5.0f + self->field_0x214;
                eft009_set_pos(0x8D, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                setVector3(&v, 0.0f, 0.0f, 0.0f);
                eft_em_spawn(self, 0x55, 1, &v, 1.0f);
            }
        }
        if (em_after_frame_check(self, 3, 48.0f, 108.0f) == 1 && (system_w.field_0x0c & 3) == 0) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 3, &v);
                eft009_set_pos(0x8E, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                eft009_spawn_at_joint(self, 0x29, 0x4E, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 0x14, &v);
                v.y = 5.0f + self->field_0x214;
                eft009_set_pos(0x8F, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.5f, self->area_no);
            } else {
                eft009_spawn_at_joint(self, 0x14, 0x4F, 0, 1.5f);
            }
        }
        if (em_after_frame_check(self, 0, 232.0f, 0.0f) == 1) {
            if (em_magma_check(self) == 1) {
                get_joint_wpos_em(self, 3, &v);
                v.y = 5.0f + self->field_0x214;
                eft009_set_pos(0x90, &v, (_CP_VECTOR*)&self->field_0x1BC, 1.0f, self->area_no);
            } else {
                eft009_spawn_at_joint(self, 3, 0x50, 0, 1.0f);
            }
        }
        if (em_after_frame_check(self, 2, 112.0f, 0.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            em_camera_req(self, -1, 0);
        }
        break;
    case 0x1A:
        if (em_after_frame_check(self, 0, 28.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 148.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
        }
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
        }
        if (em_after_frame_check(self, 0, 126.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 0, 0x23, 0, 1.5f);
        }
        if (em_after_frame_check(self, 0, 128.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x1C:
        if (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 4, 0x1C, 0, 2.0f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x1D:
        if (em_after_frame_check(self, 0, 92.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 4, 0x23, 0, 2.0f);
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x20:
        if (em_after_frame_check(self, 0, 24.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
        }
        break;
    case 0x21:
        if (em_after_frame_check(self, 0, 26.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, 0x1C, 7);
        }
        if (em_after_frame_check(self, 0, 56.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
            em_camera_req(self, 0x23, 7);
        }
        break;
    case 0x26:
        if (em_after_frame_check(self, 0, 38.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x14, 0x72, 0, 0.8f);
            em_camera_req(self, 0x14, 7);
        }
        if (em_after_frame_check(self, 0, 58.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x14, 0x71, 0xC000, 1.0f);
        }
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x14, 0x72, 0, 0.8f);
            em_camera_req(self, 0x14, 7);
        }
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x14, 0x71, 0x4000, 1.0f);
        }
        if (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 0x14, 0x72, 0, 1.0f);
            em_camera_req(self, 0x14, 7);
        }
        if (em_after_frame_check(self, 3, 178.0f, 298.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            eft009_spawn_at_joint(self, 0x14, 0x73, 0, 1.0f);
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 3, 178.0f, 288.0f) == 1 && (system_w.field_0x0c & 7) == 0) {
            em_camera_req(self, -1, 7);
        }
        break;
    case 0x27:
        if (em_after_frame_check(self, 0, 70.0f, 0.0f) == 1) {
            eft009_spawn_at_joint(self, 3, 0x70, 0, get_em_scale(self));
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 178.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, 0x1C, 7);
        }
        if (em_after_frame_check(self, 0, 228.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
            em_camera_req(self, 0x23, 7);
        }
        break;
    case 0x64:
        if (em_after_frame_check(self, 0, 34.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, -1, 7);
        }
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
        }
        break;
    case 0x66:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
        }
        break;
    case 0x67:
        if (em_after_frame_check(self, 0, 44.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 54.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
        }
        break;
    case 0x68:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 1, 0, 3, 0xC000, 1.5f);
            em_camera_req(self, -1, 5);
        }
        break;
    case 0x69:
        if (em_after_frame_check(self, 0, 50.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 1, 0, 3, 0x4000, 1.5f);
            em_camera_req(self, -1, 5);
        }
        break;
    case 0x6B:
        if (em_after_frame_check(self, 0, 30.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.7f);
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 46.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 1.7f);
        }
        break;
    case 0x6C:
        if (em_after_frame_check(self, 0, 158.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em009_effect_spawn(self, 0, 0x11, 3, 0, 1.5f);
            } else {
                em009_effect_spawn(self, 1, 5, 3, 0, 0.8f);
            }
            em_camera_req(self, -1, 5);
        }
        if (em_after_frame_check(self, 0, 440.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 1, 1, 3, 0, 1.3f);
            em_camera_req(self, -1, 5);
        }
        break;
    case 0x6D:
        if (em_after_frame_check(self, 0, 194.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 1, 0, 3, 0xC000, 1.5f);
        }
        break;
    case 0x70:
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 2.0f);
        }
        break;
    case 0x71:
        if (em_after_frame_check(self, 0, 100.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 2.0f);
        }
        break;
    case 0x75:
        if (em_after_frame_check(self, 0, 20.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em009_effect_spawn(self, 0, 0x14, 3, 0, 2.0f);
            } else {
                em009_effect_spawn(self, 1, 2, 3, 0, 1.3f);
            }
            em_camera_req(self, -1, 5);
        }
        break;
    case 0x76:
        if (em_after_frame_check(self, 0, 86.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 180.0f);
            if ((self->field_0x228 & 6) != 0) {
                eft009_spawn_at_pos(self, 0x14, 0xC, 0, 2.5f, &v);
                eft009_spawn_at_pos(self, 0x14, 0xE, 0, 2.5f, &v);
            } else {
                eft009_spawn_at_pos(self, 0x14, 2, 0, 2.5f, &v);
            }
        }
        break;
    case 0x77:
        if (em_after_frame_check(self, 0, 110.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em009_effect_spawn(self, 0, 0x10, 0x12, 0, 1.0f);
            } else {
                em009_effect_spawn(self, 1, 1, 0x12, 0, 0.7f);
            }
            em_camera_req(self, -1, 1);
        }
        break;
    case 0x7A:
        if (em_after_frame_check(self, 0, 66.0f, 0.0f) == 1) {
            if ((self->field_0x228 & 6) != 0) {
                em009_effect_spawn(self, 0, 0x11, 3, 0, 1.5f);
            } else {
                em009_effect_spawn(self, 1, 5, 3, 0, 0.9f);
            }
        }
        break;
    case 0x7B:
        if (em_after_frame_check(self, 0, 36.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 2.0f);
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 52.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 1.5f);
        }
        break;
    case 0x7C:
        if (em_after_frame_check(self, 0, 42.0f, 0.0f) == 1) {
            em_camera_req(self, -1, 1);
        }
        if (em_after_frame_check(self, 0, 146.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x1C, 0, 2.0f);
            em_camera_req(self, 0x1C, 7);
        }
        if (em_after_frame_check(self, 0, 198.0f, 0.0f) == 1) {
            em009_effect_spawn(self, 0, 2, 0x23, 0, 2.0f);
            em_camera_req(self, 0x23, 7);
        }
        break;
    case 0xC8:
        if (em_after_frame_check(self, 0, 72.0f, 0.0f) == 1 || em_after_frame_check(self, 0, 134.0f, 0.0f) == 1) {
            setVector3(&v, 0.0f, 0.0f, 100.0f);
            eft009_spawn_at_pos(self, 0x14, 0x81, 0, 1.0f, &v);
        }
        break;
    }
}

/* 0x8038E8E8 (0x4): The empty colour slot. */
void em009_tev_color_update(void) {
}

/* 0x8038E8EC (0x6C): The state query: kind 0 asks whether the +0x338 counter is still below 1800, kind 1 whether
 * the work stands outside lava. */
u32 em009_state_query(_ENEMY_WORK* self, u8 kind) {
    switch (kind) {
    case 0:
        return self->init_0x328.field_0x338 < 1800;
    case 1:
        return em_magma_check(self) != 1;
    }
    return 0;
}

/* 0x8038E958 (0x4C): The death slot: move mode 4, then action 12 sub-state 0. */
void em009_death_action_get(_ENEMY_WORK* work, u8* kind, u8* value) {
    em_move_mode_set(work, 4);
    *kind = 12;
    *value = 0;
}

/* 0x8038E9A4 (0xC): Clears the +0x33A cooldown. */
void em009_cooldown_clear(_ENEMY_WORK* work) {
    work->init_0x328.field_0x33A = 0;
}

/* 0x8038E9B0 (0x40): Whether part 6 has reached damage level 1. */
int em009_part_broken_ck(_ENEMY_WORK* work) {
    return em_parts_damage_level_get(work, 6) >= 1;
}

/* 0x8038E9F0 (0x1F8): The action selection of map kind 5: the part-break reply, the table pick, the pack call when
 * another live team-9 work shares the area, then the common selectors. */
u32 em009_action_select(_ENEMY_WORK* self, u32 arg) {
    u8 kind = stage_map_kind_get(self->field_0x1E0);
    u32 probe;
    struct _ENEMY_WORK* work;
    u16 max;
    u16 i;
    u32 hit;
    u32 found;

    switch (kind) {
    case 5:
        break;
    default:
        return 0;
    }
    hit = 0;
    probe = (s32)kind == 5 ? 5 : 0xFF;
    if (probe != 0xFF) {
        switch (em_area_break_ck_c2(self, probe)) {
        case 1:
            hit = 1;
            break;
        case 2:
            return 1;
        }
    }
    if (hit == 0 && self->value_0x452 >= 900) {
        if (em_pack_area_pick_c1(self, 0, (s32)kind == 5 ? (u8)6 : (u8)0xFF, (u16)arg, 0, em009_pick_tbl) == 1) {
            return 1;
        }
    }
    if (hit == 0 && self->value_0x452 >= 900) {
        found = 0;
        work = (struct _ENEMY_WORK*)get_move_work_adrs(3);
        max = get_move_work_max_c1(3);
        for (i = 0; i < max; i++, work++) {
            if (self->group != i && em_work_die_ck_c1(work) != 1 && work->team == 9 &&
                work->area_no == self->area_no) {
                found = 1;
                break;
            }
        }
        if (found == 1 && em_roll_cooldown_ck_c1(self, (u16)arg, 0x46, &self->init_0x328.field_0x33A) == 1) {
            self->field_0x1FC = 1;
            self->field_0x1FE = 2;
            self->field_0x1FF = 0xFF;
            return 1;
        }
    }
    if (em_area_roam_pick(self, (u16)arg) == 1) {
        return 1;
    }
    return (em_area_leave_ck(self) - 1) == 0;
}


} /* extern "C" */

/* 0x8038EBE8 (0x5C): Destroys the accessor through the base destructor (the class table is emitted with it). */
Em009UserData::~Em009UserData() {
}
