/* enemy/fn_801B7020.cpp - the em036 enemy's program: the `em036_prog_tbl` entry points (init, frame update, action
 *   dispatch, motion-next, event, motion events, tint, state query, area start, gauge, state-clear check, mini init),
 *   the run-in/hop/swing motions of actions 0..3, 7 and 12, the sound programs of actions 10/11 and the static
 *   initializer of its vector pair.
 * RANGE. .text 0x801B7020-0x801B98C8 (50 functions); .ctors 0x8056F34C-0x8056F350 (the static initializer),
 *   .data 0x805B2118-0x805B2804 (from `em036_prog_tbl`), .bss 0x806A7AB8-0x806A7AD0, .sdata 0x80791AB0-0x80791AB8,
 *   .sdata2 0x80798CF8-0x80798D88, extab, extabindex.
 * SEAM. The band 0x801B7020-0x801E0ADC is six TUs: each one's `.data` opens with its `emNNN_prog_tbl`, each ends with
 *   its static initializer (the `.ctors` words 0x8056F34C/350/354/358) and the `.sdata2` pool repeats a value at each
 *   change - em036 here, em040 `enemy/em040_ai.cpp`, em006 `enemy/em006_prog.cpp`, em004 `enemy/em004_act.cpp`, em005
 *   `enemy/em005_act.cpp`, em007 `enemy/em007_act.cpp`.  Right edge: 0x801B985C is this TU's static initializer and
 *   the 0.0 pool entry repeats at 0x80798DA4 from 0x801B98C8 on; left edge: the 4-byte `em034_eft_ck` sits in the
 *   0.0 dedupe window 0x801B701C-0x801B70A4 and stays with `enemy/em034_prog.cpp` (unproven either way).
 * FLAGS. `cflags_main`; `#pragma peephole off` file-wide (retail keeps `clrlwi`/`rlwinm` + `cmpwi` pairs) and
 *   `#pragma pool_data off`.
 * NAMES. The file keeps the map stem: the dump answers `zz_01b7020_` for 0x801B7020 and no `__FILE__` string is
 *   referenced; em036 is a GUESS from the prog table that opens the TU's data (`enemy/em036_prog.cpp` is the proposed
 *   name).  GUESS (the `Em036ProgTbl` slot each fills, after `Em008ProgTbl`/`Em040ProgTbl`): em036_init,
 *   em036_frame_update, em036_action_dispatch, em036_motion_next, em036_event_notify, em036_motion_events,
 *   em036_tint_update, em036_state_ck, em036_area_start_set, em036_gauge_update, em036_state_clear_ck,
 *   em036_mini_init.  GUESS (from the bodies): em036_height_descend, em036_floor_clamp, em036_forward_move_set,
 *   em036_ground_variant_roll (em027's twin), em036_act<action>_<sub-state>, em036_act<action>_dispatch,
 *   em036_act10_se_play, em036_act11_se_play; data: em036_hop_y_curve, em036_hop_z_curve, em036_swing_y_curve,
 *   em036_swing_z_curve, em036_se_* (named after the first sub-state that plays each), em036_se_shift,
 *   em036_vec_pair; the callee `em_pl_hate_get` (0x8012C0EC, registry).
 * RESIDUALS. none in `.text`, `.data`, `.sdata`, `.bss`, `.ctors`; `.sdata2` is 4 B short of the claim (0x8C vs 0x90: the
 *   trailing alignment pad after the last pool word, which the link supplies before em040's 8-aligned pool).
 *   flipcheck: the callee `em_pl_hate_get` (0x8012C0EC) is the name registry's proposal (lane enemy-801b7020) and
 *   is not a map name yet, so a flip links `undefined` until it lands.
 * SHAPES. `#pragma pool_data off` (each sound table is addressed with its own `lis`/`addi`).  `self->state++`, not
 *   `self->state = self->state + 1`; a `switch` for the sparse dispatches; `(s32)(u8)mode` for the gates retail
 *   compares with `cmpwi`.  A sign flip after a store is `field *= -1.0f` (the multiply then reads the stored
 *   register first, `fmuls f0,f1,f0`); a mode-picked argument is a local set before an `if`, not a ternary; the
 *   motion-event flag updates are one statement per bit (a single expression folds the masks); the tint's colour
 *   bytes take the float product directly (an `(s32)` cast adds a `clrlwi`); `em_move_vec_set` takes its vector by
 *   value (the caller's stack copy is the r3/r0 pair copy).  `EmProgWork` is this unit's view because it keeps an
 *   `f32` at +0x328 where `enemy/ENEMY_WORK.h` has an `s16`.  The static initializer is MWCC's own `__sinit` (a
 *   class with an inline constructor), so the `.ctors` word is the compiler's.
 */
#include "types.h"

#include "nw4r/math.h"

/* Retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs the peephole pass folds into record forms
 * (`em036_floor_clamp`'s `joint_flags` test, `em036_motion_next`'s `bits_0x1EC` test; playbook 39). */
#pragma peephole off
/* Each table is addressed by its own `lis`/`addi` pair, not through a shared `.data` base. */
#pragma pool_data off

#include "mh3_pad.h"
#include "gx.h"
#include "pl.h"

#include "ef/eft007.h"
#include "ef/fn_800CDB2C.h"
#include "fn_8004CAD8.h"
#include "stage/shell_set_func_ptr.h"

#include "enemy/ENEMY_MINI_WORK.h"
#include "enemy/enemy_control.h"
#include "enemy/em_common.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/em_pl_hate_get.h"
#include "enemy/em_se_record.h"
#include "enemy/em_sight_player_ck2.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_80147CE0.h"
#include "enemy/fn_8012BDF4.h"
#include "enemy/fn_8012EC74.h"

#include "unsplit/enemy.h"

/* The C++-mangled callees, declared at C++ scope for the map's manglings; `rotVecY` (0x80051064) is
 * `fn_8004CAD8.cpp`'s. */
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* ------------------------------------------------------------------------------------------------ */
/* this range's view of the records it reads                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* The 0x20-byte out record `em_ground_rec_clear` prepares and `em_ground_rec_find` fills: the mode byte at +0x03
 * (`em036_ground_variant_roll`), the position at +0x08 and the u32 at +0x18 `em036_init` narrows to u16.
 * size: 0x20 */
struct EmSelRec {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 mode;          /* 0 or 2: the random threshold `em036_ground_variant_roll` picks */
    /* +0x04 */ u8 unused_0x04[0x04];
    /* +0x08 */ nw4r::math::VEC3 vec_0x08; /* the ground position `em036_area_start_set` places the work at */
    /* +0x14 */ u8 unused_0x14[0x04];
    /* +0x18 */ u32 value_0x18;   /* the id `em036_init` latches into the work's +0x32E */
};

/* The enemy work record, as this range's accesses measure it.  `state` (+0x05) is the per-motion step
 * every dispatcher in the range switches on, `timer_0x020` the countdown they run down, and the
 * +0x328..+0x332 cluster the per-action distance/timer state.
 * size: 0xB18 */
struct EmProgWork {
    /* +0x000 */ u8 active;              /* nonzero while the record is in use */
    /* +0x001 */ u8 unused_0x001[0x005 - 0x001];
    /* +0x005 */ u8 state;               /* the per-motion step (0..5) the dispatchers switch on */
    /* +0x006 */ u8 field_0x006;         /* 1 once `em_approach_step` reports the approach done */
    /* +0x007 */ u8 field_0x007;         /* 1 once the height is within 50 of the target's */
    /* +0x008 */ u8 unused_0x008[0x00A - 0x008];
    /* +0x00A */ u8 field_0x00A;         /* bit 1: the random-motion flag `em036_ground_variant_roll` feeds */
    /* +0x00B */ u8 unused_0x00B[0x00F - 0x00B];
    /* +0x00F */ u8 field_0x00F;
    /* +0x010 */ u8 field_0x010;
    /* +0x011 */ u8 unused_0x011[0x01A - 0x011];
    /* +0x01A */ u16 field_0x01A;        /* the id `em_ground_rec_find` is handed */
    /* +0x01C */ u8 unused_0x01C[0x020 - 0x01C];
    /* +0x020 */ s32 timer_0x020;        /* the step countdown (`bgt` keeps a positive one running) */
    /* +0x024 */ u8 char_0x024[0x34];    /* the embedded `MHchar` the tint reads and writes */
    /* +0x058 */ u8 unused_0x058[0x188 - 0x058];
    /* +0x188 */ nw4r::math::VEC3 pos;   /* +0x18C is the height the range clamps */
    /* +0x194 */ u8 unused_0x194[0x1B0 - 0x194];
    /* +0x1B0 */ nw4r::math::VEC3 aim;   /* the point the action-12 return run starts from */
    /* +0x1BC */ u32 field_0x1BC;        /* rotation angle (wraps at 0x10000) */
    /* +0x1C0 */ u32 field_0x1C0;        /* the Y rotation `rotVecY` is handed */
    /* +0x1C4 */ u32 field_0x1C4;
    /* +0x1C8 */ u8 unused_0x1C8[0x1CC - 0x1C8];
    /* +0x1CC */ f32 value_0x1CC;        /* armed with 1.0f by the charge branch */
    /* +0x1D0 */ u8 unused_0x1D0[0x1E1 - 0x1D0];
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 field_0x1E2;         /* 1: `em036_state_clear_ck` reports the state clear */
    /* +0x1E3 */ u8 unused_0x1E3[0x1E5 - 0x1E3];
    /* +0x1E5 */ u8 action;              /* the action `em036_action_dispatch` switches on */
    /* +0x1E6 */ u8 state_sub;           /* the sub-state several dispatchers switch on */
    /* +0x1E7 */ u8 unused_0x1E7[0x1EC - 0x1E7];
    /* +0x1EC */ u16 bits_0x1EC;         /* low 3 bits gate `em036_motion_next`; the hop picks its offsets from it */
    /* +0x1EE */ u8 unused_0x1EE[0x210 - 0x1EE];
    /* +0x210 */ f32 value_0x210;        /* the base of the walk-in height clamp */
    /* +0x214 */ u8 unused_0x214[0x228 - 0x214];
    /* +0x228 */ u16 joint_flags;        /* bits 2-3 gate the walk-in clamp */
    /* +0x22A */ u8 unused_0x22A[0x310 - 0x22A];
    /* +0x310 */ nw4r::math::VEC3 vec_0x310; /* the run-in vector */
    /* +0x31C */ nw4r::math::VEC3 vec_0x31C; /* the hop's saved run-in vector; .y is armed with -1.0f */
    /* +0x328 */ f32 value_0x328;        /* the walk-in/charge distance the motion mode picks */
    /* +0x32C */ u8 field_0x32C;         /* cleared on entry by `em036_init` */
    /* +0x32D */ u8 unused_0x32D[0x32E - 0x32D];
    /* +0x32E */ u16 field_0x32E;        /* latched from the selection record */
    /* +0x330 */ s16 tint_0x330;         /* the tint level: 0xFF, or 150 for the random motion; 150..255 */
    /* +0x332 */ u8 unused_0x332[0x36C - 0x332];
    /* +0x36C */ nw4r::math::VEC3 target; /* the height the clamp measures against (+0x370) */
    /* +0x378 */ u8 unused_0x378[0x38B - 0x378];
    /* +0x38B */ u8 field_0x38B;         /* cleared once `em034_eft_ck` reports done */
    /* +0x38C */ u8 unused_0x38C[0x43B - 0x38C];
    /* +0x43B */ u8 field_0x43B;         /* the action/mode byte `em_motion_kind_set` compares */
    /* +0x43C */ u8 unused_0x43C[0x440 - 0x43C];
    /* +0x440 */ s16 field_0x440;        /* the 150-frame timer `em036_frame_update` tests */
    /* +0x442 */ u8 unused_0x442[0x761 - 0x442];
    /* +0x761 */ u8 field_0x761;         /* bits 0..2: the motion-event flags `em036_motion_events` sets */
    /* +0x762 */ u8 unused_0x762[0x834 - 0x762];
    /* +0x834 */ u8 field_0x834;
    /* +0x835 */ u8 unused_0x835[0xB14 - 0x835];
    /* +0xB14 */ u32 field_0xB14;
};

/* A point and its per-frame step, built by MWCC's `__sinit` (0x801B985C): each vector is set through `setVec3` into
 * a temporary and copied with `assignVec3`.  size: 0x18 */
struct Em036VecPair {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ VEC3 step;

    /* Builds (0, 0, z) and (0, 0, 0). */
    Em036VecPair(f32 z)
    {
        VEC3 b;
        VEC3 a;

        assignVec3((Vec*)&pos, (Vec*)setVec3(&a, 0.0f, 0.0f, z));
        assignVec3((Vec*)&step, (Vec*)setVec3(&b, 0.0f, 0.0f, 0.0f));
    }
};

/* The vector pair (`.bss`, defined at the end); a sound record points at it. */
extern Em036VecPair em036_vec_pair;

/* `vec3_sub_assign` subtracts `b` from `self` in place (`ef/ef_postfield.cpp`, whose header clashes with `mh3_pad.h`'s
 * `VEC3_ctor`/`setVec3`). */
extern "C" {
void vec3_sub_assign(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);
/* 0x80051EE0 (`fn_8004CAD8.cpp`): r3 `out`, r4 `in`, f1 the scale it keeps in f31 while `VEC3_ctor` zeroes `out`;
 * the tree spells it with three and four arguments, so it is declared here. */
void vec3_scale(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 scale);
}

/* `em_move_vec_set` (`enemy/em_common.cpp`) called with the vector by value: the caller copies it into a stack
 * temporary and passes that temporary's address. */
#define em_move_vec_set_c1 ((void (*)(struct _ENEMY_WORK*, nw4r::math::VEC3))em_move_vec_set)

/* The shell job slot +0x3C, called without its table argument. */
typedef void (*ShellRecJobFn)(struct _ENEMY_WORK* self, EmSpawnRec* rec, u32 mode);

/* The em036 program table (`em036_prog_tbl`): the entry points the shared enemy program driver calls (slot names
 * follow `Em008ProgTbl`/`Em010ProgTbl`; the empty slots are zero).
 * size: 0x70 */
struct Em036ProgTbl {
    /* +0x00 */ void (*work_clear_0x00)(EmProgWork* self);
    /* +0x04 */ void (*init_0x04)(EmProgWork* self, u8 mode);
    /* +0x08 */ void (*frame_update_0x08)(EmProgWork* self);
    /* +0x0C */ void (*action_dispatch_0x0C)(EmProgWork* self);
    /* +0x10 */ void (*motion_next_0x10)(EmProgWork* self, u8* record, u8* value);
    /* +0x14 */ void (*event_notify_0x14)(void);
    /* +0x18 */ void (*motion_events_0x18)(EmProgWork* self);
    /* +0x1C */ u32 pad_0x1C;
    /* +0x20 */ void (*tint_update_0x20)(EmProgWork* self);
    /* +0x24 */ s32 (*state_ck_0x24)(void);
    /* +0x28 */ u32 pad_0x28[2];
    /* +0x30 */ void (*area_start_set_0x30)(EmProgWork* self, u8* action, u8* sub);
    /* +0x34 */ void (*gauge_update_0x34)(void);
    /* +0x38 */ u32 pad_0x38[5];
    /* +0x4C */ s32 (*state_clear_ck_0x4C)(EmProgWork* self);
    /* +0x50 */ u32 pad_0x50[3];
    /* +0x5C */ void (*mini_init_0x5C)(_ENEMY_MINI_WORK* mini, u8 mode);
    /* +0x60 */ u32 pad_0x60[4];
};

extern "C" {

void em036_height_descend(EmProgWork* self);
void em036_floor_clamp(EmProgWork* self);
void em036_forward_move_set(EmProgWork* self);
s32 em036_ground_variant_roll(u16 id);
void em036_init(EmProgWork* self, u8 mode);
void em036_motion_next(EmProgWork* self, u8* record, u8* value);
void em036_event_notify(void);
void em036_frame_update(EmProgWork* self);
void em036_act2_0(EmProgWork* self);
void em036_action_dispatch(EmProgWork* self);
void em036_motion_events(EmProgWork* self);
void em036_tint_update(EmProgWork* self);
s32 em036_state_ck(void);
void em036_area_start_set(EmProgWork* self, u8* action, u8* sub);
void em036_gauge_update(void);
s32 em036_state_clear_ck(EmProgWork* self);
void em036_mini_init(_ENEMY_MINI_WORK* mini, u8 mode);

Em036ProgTbl em036_prog_tbl = {
    0,
    em036_init,
    em036_frame_update,
    em036_action_dispatch,
    em036_motion_next,
    em036_event_notify,
    em036_motion_events,
    0,
    em036_tint_update,
    em036_state_ck,
    {0, 0},
    em036_area_start_set,
    em036_gauge_update,
    {0, 0, 0, 0, 0},
    em036_state_clear_ck,
    {0, 0, 0},
    em036_mini_init,
    {0, 0, 0, 0},
};

/* The (frame, value) key curves `em_key_curve_eval` reads for the run-in vector: the hop's height and advance
 * (`em036_act2_0`, `em036_act3_12`) and the swing's (`em036_act7_0`); a frame of -1 ends each. */
f32 em036_hop_y_curve[24] = {
    0.0f, 0.0f, 6.0f, 0.0f, 8.0f, 30.5f, 10.0f, 16.0f, 12.0f, 8.5f, 16.0f, 1.75f,
    22.0f, -2.33f, 26.0f, -6.75f, 28.0f, -12.5f, 32.0f, -12.75f, 34.0f, 0.0f, -1.0f, 0.0f,
};

f32 em036_hop_z_curve[24] = {
    0.0f, 0.0f, 6.0f, 0.0f, 8.0f, 32.5f, 10.0f, 21.5f, 12.0f, 16.0f, 16.0f, 11.25f,
    22.0f, 14.0f, 26.0f, 17.0f, 28.0f, 18.0f, 32.0f, -93.25f, 34.0f, 0.0f, -1.0f, 0.0f,
};

f32 em036_swing_y_curve[28] = {
    0.0f, 0.0f, 4.0f, 2.5f, 6.0f, -5.0f, 10.0f, -9.0f, 20.0f, -10.4f, 26.0f, -7.67f, 30.0f, -3.5f,
    34.0f, 1.25f, 40.0f, 3.33f, 46.0f, 5.83f, 54.0f, 6.75f, 60.0f, 7.17f, 68.0f, 5.38f, -1.0f, 5.38f,
};

f32 em036_swing_z_curve[28] = {
    0.0f, 0.0f, 4.0f, 10.0f, 6.0f, 14.0f, 10.0f, 18.0f, 20.0f, 20.4f, 26.0f, 13.33f, 30.0f, 14.0f,
    34.0f, 15.0f, 40.0f, 12.67f, 46.0f, 10.67f, 54.0f, 8.75f, 60.0f, 4.0f, 68.0f, 3.25f, -1.0f, 3.25f,
};

/* 0x801B7020 (0x28): while the work's height (`pos.y`) is more than 1500 above its target height (`target.y`),
 * drops it by 20 per frame. */
void em036_height_descend(EmProgWork* self) {
    f32 height = self->pos.y;
    if (height > 1500.0f + self->target.y) {
        self->pos.y = height - 20.0f;
    }
}

/* 0x801B7048 (0x5C): for the part flags that walk in (bits 2-3 of `joint_flags`), keeps the height at least
 * `value_0x210` plus 30 units of the model scale. */
void em036_floor_clamp(EmProgWork* self) {
    if (self->joint_flags & 0x6) {
        f32 step = 30.0f * get_em_scale((struct _ENEMY_WORK*)self);
        f32 height;
        height = self->value_0x210 + step;
        if (self->pos.y < height) {
            self->pos.y = height;
        }
    }
}

/* 0x801B70A4 (0x74): rotates a 45-unit forward vector by the work's Y rotation and hands it to the move setter. */
void em036_forward_move_set(EmProgWork* self) {
    nw4r::math::VEC3 rot;

    VEC3_ctor(&rot);
    rot.x = 0.0f;
    rot.y = 0.0f;
    rot.z = 45.0f;
    rotVecY(&rot, self->field_0x1C0);
    em_move_vec_set_c1((struct _ENEMY_WORK*)self, rot);
}

/* 0x801B7118 (0xDC): picks the random-motion kind for ground record `id`: 0 = no record, 1/2 = the kind the
 * record's mode chooses (mode 0: the 10 % branch, mode 2: the 60 % branch). */
s32 em036_ground_variant_roll(u16 id) {
    EmSelRec rec;
    s32 roll;

    em_ground_rec_clear(&rec);
    if (move_work_state_ck() == 0) {
        return 0;
    }
    if (em_ground_rec_find((u16)id, &rec) == 0) {
        return 0;
    }
    roll = (u16)ran_suu(0) % 100;
    if (rec.mode == 2) {
        return (roll < 60) + 1;
    }
    if (rec.mode == 0) {
        return (roll < 10) + 1;
    }
    return 0;
}

/* 0x801B71F4 (0x1AC): latches the selection record, then either rolls the random kind (modes 1 and 4) or restarts
 * the motion set, and picks the walk-in distance from the kind. */
void em036_init(EmProgWork* self, u8 mode) {
    EmSelRec rec;
    s32 pick;

    em_ground_rec_clear(&rec);
    self->field_0x32C = 0;
    if (em_ground_rec_find(self->field_0x01A, &rec) == 1) {
        self->field_0x32E = (u16)rec.value_0x18;
    } else {
        self->field_0x32E = 0;
    }
    self->tint_0x330 = 0xFF;
    if ((s32)(u8)mode == 1 || (s32)(u8)mode == 4) {
        pick = (s8)em036_ground_variant_roll(self->field_0x01A);
        if (pick == 1) {
            self->field_0x00A &= 0xFD;
        } else if (pick == 2) {
            self->field_0x00A |= 0x02;
        }
    } else {
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        if (self->field_0x00A & 1) {
            em_act_arm_unless_down((struct _ENEMY_WORK*)self, 3, 10);
        } else {
            em_act_arm_unless_down((struct _ENEMY_WORK*)self, 0, 3);
        }
        if (self->field_0x00F == 0) {
            pick = (s8)em036_ground_variant_roll(self->field_0x01A);
            if (pick == 1) {
                self->field_0x00A &= 0xFD;
            } else if (pick == 2) {
                self->field_0x00A |= 0x02;
            }
        }
    }
    if (self->field_0x00A & 2) {
        self->value_0x328 = 3000.0f;
        self->value_0x1CC = 1.0f;
        em_max_hp_set((struct _ENEMY_WORK*)self, 80);
    } else if (self->field_0x00A & 1) {
        self->value_0x328 = 1200.0f;
        self->tint_0x330 = 150;
    } else {
        self->value_0x328 = 1500.0f;
    }
}

/* 0x801B73A0 (0x4C): retargets the next motion (r4 the `{ code, ... }` record, r5 the value): only the code 11 row
 * with an empty 3-bit `bits_0x1EC` field runs, moving the value 0 -> 29 and 5 -> 30. */
void em036_motion_next(EmProgWork* self, u8* record, u8* value) {
    if (*record != 11) {
        return;
    }
    if (self->bits_0x1EC & 7) {
        return;
    }
    switch (*value) {
    case 0:
        *value = 29;
        break;
    case 5:
        *value = 30;
        break;
    }
}

/* 0x801B73EC (0x4): the event slot; em036 has none. */
void em036_event_notify(void) {
}

/* 0x801B73F0 (0xA4): clears the "already reported" byte once the program reports done, then shuts the action down
 * when the work is not dead and its 150-frame timer has run out. */
void em036_frame_update(EmProgWork* self) {
    if (em034_eft_ck((struct _ENEMY_WORK*)self) == 0 && self->field_0x38B == 1) {
        self->field_0x38B = 0;
    }
    if (em_die_ck((struct _ENEMY_WORK*)self)) {
        return;
    }
    if (self->field_0x834 == 1) {
        em_motion_kind_set((struct _ENEMY_WORK*)self, 2, 0);
    }
    if (self->field_0x43B == 2 && self->field_0x440 > 150) {
        em_motion_kind_set((struct _ENEMY_WORK*)self, 0, 0);
        self->field_0x834 = 0;
    }
}

/* 0x801B7494 (0x7C): step 0 arms the motion set (`em_move_mode_set` with mode 0 and the 4/4/0 window), step 1
 * hands over to `em_action_finish` once the motion ends. */
void em036_act0_0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7510 (0x80): the same two-step shape with the motion set restarted and the 1/4/0 window, handing over to
 * `em_action_finish_fall`. */
void em036_act0_3(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7590 (0x3C): action 0's sub-state dispatcher. */
void em036_act0_dispatch(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em036_act0_0(self);
        break;
    case 1:
        em036_act0_0(self);
        break;
    case 2:
        em036_act0_0(self);
        break;
    case 3:
        em036_act0_3(self);
        break;
    }
}

/* 0x801B75CC (0x7C): a second copy of action 0's first motion (the step bodies are byte-identical). */
void em036_act1_0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 4, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7648 (0x7C): the same shape with the 12/2/0 window. */
void em036_act1_1(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 12, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B76C4 (0xF4): the three-step drop: step 0 restarts the motion set and arms the 101/2/0 window and the fall
 * vector, step 1 waits for `em_ground_ck`, step 2 ends the motion. */
void em036_act1_2(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 101, 2, 0);
        em_dive_start((struct _ENEMY_WORK*)self);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = -5.0f;
        self->vec_0x31C.y = -1.0f;
        break;
    case 1:
        CancelFade((struct _ENEMY_WORK*)self);
        em_fall_height_get((struct _ENEMY_WORK*)self);
        if (em_ground_ck((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            em_move_mode_set((struct _ENEMY_WORK*)self, 0);
            em_mot_set((struct _ENEMY_WORK*)self, 102, 2, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B77B8 (0x7C): the two-step shape with the 103/2/0 window. */
void em036_act1_3(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 103, 2, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7834 (0x7C): the two-step shape with the 107/4/0 window through `em_mot_set`. */
void em036_act1_4(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set((struct _ENEMY_WORK*)self, 107, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B78B0 (0x48): action 1's sub-state dispatcher. */
void em036_act1_dispatch(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em036_act1_0(self);
        break;
    case 1:
        em036_act1_1(self);
        break;
    case 2:
        em036_act1_2(self);
        break;
    case 3:
        em036_act1_3(self);
        break;
    case 4:
        em036_act1_4(self);
        break;
    }
}

/* 0x801B78F8 (0x15C): the hop: step 0 arms the 5/4/0 window, step 1 starts the height curve at frame 6, step 2
 * follows it until frame 16 and step 3 times the last 10 frames out. */
void em036_act2_0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set((struct _ENEMY_WORK*)self, 0);
        em_mot_set((struct _ENEMY_WORK*)self, 5, 4, 0);
        break;
    case 1:
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 6.0f, 0.0f) == 1) {
            self->state++;
            em_fall_height_get((struct _ENEMY_WORK*)self);
            em_fall_start((struct _ENEMY_WORK*)self);
            em_move_vec2_clr((struct _ENEMY_WORK*)self);
            self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, em036_hop_y_curve);
            em_move_offset_apply((struct _ENEMY_WORK*)self);
        }
        break;
    case 2:
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, em036_hop_y_curve);
        em_move_offset_apply((struct _ENEMY_WORK*)self);
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 16.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set((struct _ENEMY_WORK*)self, 1, 10, 0);
            self->timer_0x020 = 10;
            em_move_vec2_clr((struct _ENEMY_WORK*)self);
        }
        break;
    case 3:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7A54 (0x14): action 2's dispatcher: sub-state 0 runs the hop. */
void em036_act2_dispatch(EmProgWork* self) {
    if (self->state_sub == 0) {
        em036_act2_0(self);
    }
}

/* 0x801B7A68 (0x174): the charge: restarts the motion set, takes the approach speed from the mode (8 frames) and
 * scales the normalised vector to the target into the run-in slot. */
void em036_act3_0(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        em_approach_start((struct _ENEMY_WORK*)self, (s32)(u8)mode == 1 ? -450.0f : -80.0f, 8);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        copyVec3(&vec, &self->target);
        vec.y += 60.0f;
        vec3_sub_assign(&vec, &self->pos);
        if (vec3_length_sq((const f32*)&vec) > 0.001f) {
            vec3_normalize_into(&vec, &vec);
            vec3_scale(&out, &vec, 8.0f);
            copyVec3(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 240;
        break;
    case 1:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        if (em_approach_step((struct _ENEMY_WORK*)self, 0, 0) == 1 || self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B7BDC (0xF8): the held run-in: the vector is kept for 100 frames (10 in the short mode) and re-aimed every
 * frame. */
void em036_act3_1(EmProgWork* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 3, 20, 0);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.z = -5.0f;
        self->timer_0x020 = 100;
        if (mode == 1) {
            self->vec_0x310.z *= 8.0f;
            self->timer_0x020 = 10;
        }
        rotVecY(&self->vec_0x310, self->field_0x1C0);
        break;
    case 1:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B7CD4 (0xDC): the dive: the 1/4/0 window, then the 6/4/0 one once the ground is reached. */
void em036_act3_2(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 1, 4, 0);
        em_dive_start((struct _ENEMY_WORK*)self);
        break;
    case 1:
        em_dive_step((struct _ENEMY_WORK*)self);
        em_fall_height_get((struct _ENEMY_WORK*)self);
        if (em_ground_ck((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            em_move_mode_set((struct _ENEMY_WORK*)self, 0);
            em_mot_set((struct _ENEMY_WORK*)self, 6, 4, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7DB0 (0x84): a two-step turn that ends as soon as `em_turn_to_target` reports the target faced. */
void em036_act3_3(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (em_turn_to_target((struct _ENEMY_WORK*)self, 0x400) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B7E34 (0x144): the charge's run-in vector with a 150-frame hold and no approach speed. */
void em036_act3_4(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        copyVec3(&vec, &self->target);
        vec.y += 60.0f;
        vec3_sub_assign(&vec, &self->pos);
        if (vec3_length_sq((const f32*)&vec) > 0.001f) {
            vec3_normalize_into(&vec, &vec);
            vec3_scale(&out, &vec, 10.0f);
            copyVec3(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 150;
        break;
    case 1:
        if (mode == 0) {
            em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        }
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B7F78 (0x214): the long zig-zag hop: step 0 aims the run-in vector from the `bits_0x1EC` offsets, steps 1..3
 * alternate a hold, a run and a reversed hold of 3 frames each (step 1 is 4..7 frames). */
void em036_act3_6(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        self->timer_0x020 = 3;
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.x = 8.0f;
        if ((self->bits_0x1EC & 1) == 0) {
            self->vec_0x310.x *= -1.0f;
        }
        self->vec_0x310.y = (s32)((self->bits_0x1EC & 7) - 4);
        self->vec_0x310.z = (s32)(((self->bits_0x1EC >> 4) & 7) - 4);
        if (self->vec_0x310.z >= 0.0f) {
            em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        } else {
            em_mot_set((struct _ENEMY_WORK*)self, 3, 20, 0);
        }
        rotVecY(&self->vec_0x310, self->field_0x1C0);
        copyVec3(&self->vec_0x31C, &self->vec_0x310);
        break;
    case 1:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        CancelFade((struct _ENEMY_WORK*)self);
        em036_height_descend(self);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            self->timer_0x020 = (self->bits_0x1EC & 3) + 4;
        }
        break;
    case 2:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        em_move_offset_apply((struct _ENEMY_WORK*)self);
        em036_height_descend(self);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            self->timer_0x020 = 3;
            vec3_scale_in_place(&self->vec_0x31C, -1.0f);
        }
        break;
    case 3:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        CancelFade((struct _ENEMY_WORK*)self);
        em036_height_descend(self);
        if (--self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B818C (0x104): the slow approach: a tenth of the vector to the target for 10 frames, then the turn-back
 * state 3/10 with the latched rotation. */
void em036_act3_9(EmProgWork* self) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 out;

    VEC3_ctor(&dir);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 1, 4, 0);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        subVec3(&diff, &self->target, &self->pos);
        copyVec3(&dir, &diff);
        vec3_scale(&out, &dir, 0.1f);
        copyVec3(&self->vec_0x310, &out);
        self->timer_0x020 = 10;
        break;
    case 1:
        em036_forward_move_set(self);
        if (self->timer_0x020 <= 0) {
            self->field_0x1C0 = self->field_0x32E;
            em_state_set((struct _ENEMY_WORK*)self, 3, 10);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
}

/* 0x801B8290 (0x174): the rest: turns toward 0xC000, and every 30 frames hands over to state 3/12 when a player in
 * sight carries a hate of at least 200. */
void em036_act3_10(EmProgWork* self) {
    u32 i;

    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set_ck((struct _ENEMY_WORK*)self, 7, 10, 0);
        em_rest_flag_set((struct _ENEMY_WORK*)self);
        self->field_0x1BC = em_angle_approach(0xC000, self->field_0x1BC, 0xC00);
        em_hit_arm_set((struct _ENEMY_WORK*)self);
        self->field_0x38B = 1;
        self->timer_0x020 = 0;
        break;
    case 1:
        self->field_0x1BC = em_angle_approach(0xC000, self->field_0x1BC, 0xC00);
        em_hit_arm_set((struct _ENEMY_WORK*)self);
        if (self->timer_0x020 % 30 == 0) {
            for (i = 0; i < 4; i++) {
                if (em_sight_player_ck2((struct _ENEMY_WORK*)self, i) != 0 &&
                    em_pl_hate_get((struct _ENEMY_WORK*)self, (u8)i) >= 200) {
                    em_state_set((struct _ENEMY_WORK*)self, 3, 12);
                    return;
                }
            }
        }
        self->timer_0x020++;
        break;
    }
    em036_forward_move_set(self);
}

/* 0x801B8404 (0x214): the short zig-zag hop: `em036_act3_6` with 2-frame holds (step 1 is 1..4 frames). */
void em036_act3_11(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        self->timer_0x020 = 2;
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.x = 8.0f;
        if ((self->bits_0x1EC & 1) == 0) {
            self->vec_0x310.x *= -1.0f;
        }
        self->vec_0x310.y = (s32)((self->bits_0x1EC & 7) - 4);
        self->vec_0x310.z = (s32)(((self->bits_0x1EC >> 4) & 7) - 4);
        if (self->vec_0x310.z >= 0.0f) {
            em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        } else {
            em_mot_set((struct _ENEMY_WORK*)self, 3, 20, 0);
        }
        rotVecY(&self->vec_0x310, self->field_0x1C0);
        copyVec3(&self->vec_0x31C, &self->vec_0x310);
        break;
    case 1:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        CancelFade((struct _ENEMY_WORK*)self);
        em036_height_descend(self);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            self->timer_0x020 = (self->bits_0x1EC & 3) + 1;
        }
        break;
    case 2:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        em_move_offset_apply((struct _ENEMY_WORK*)self);
        em036_height_descend(self);
        if (--self->timer_0x020 <= 0) {
            self->state++;
            self->timer_0x020 = 2;
            vec3_scale_in_place(&self->vec_0x31C, -1.0f);
        }
        break;
    case 3:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        CancelFade((struct _ENEMY_WORK*)self);
        em036_height_descend(self);
        if (--self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B8618 (0x188): the hop with an advance: the height and advance curves from frame 6 to frame 16, then the
 * 10-frame landing. */
void em036_act3_12(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 5, 4, 0);
        break;
    case 1:
        em036_forward_move_set(self);
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 6.0f, 0.0f) == 1) {
            self->state++;
            em_move_vec2_clr((struct _ENEMY_WORK*)self);
            self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, em036_hop_y_curve);
            self->vec_0x310.z = -em_key_curve_eval((struct _ENEMY_WORK*)self, em036_hop_z_curve);
            em_move_offset_apply((struct _ENEMY_WORK*)self);
        }
        break;
    case 2:
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, em036_hop_y_curve);
        self->vec_0x310.z = -em_key_curve_eval((struct _ENEMY_WORK*)self, em036_hop_z_curve);
        em_move_offset_apply((struct _ENEMY_WORK*)self);
        if (em_frame_check((struct _ENEMY_WORK*)self, 1, 16.0f, 0.0f) == 1) {
            self->state++;
            em_mot_set((struct _ENEMY_WORK*)self, 1, 10, 0);
            self->timer_0x020 = 10;
            em_move_vec2_clr((struct _ENEMY_WORK*)self);
        }
        break;
    case 3:
        self->timer_0x020--;
        if (self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B87A0 (0x80): the two-step shape with the motion set restarted and the 1/4/0 window. */
void em036_act3_13(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 1, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B8820 (0x220): the climbing charge: the charge's run-in vector while the height steps 1 unit a frame toward
 * 60 above the target's, ending once both the approach and the height are done (or after 240 frames). */
void em036_act3_15(EmProgWork* self, u8 mode) {
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 out;
    f32 speed;
    f32 gap;

    VEC3_ctor(&vec);
    switch (self->state) {
    case 0:
        self->state++;
        self->field_0x006 = 0;
        self->field_0x007 = 0;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 2, 20, 0);
        speed = 0.0f;
        if (mode == 1) {
            speed = -450.0f;
        }
        em_approach_start((struct _ENEMY_WORK*)self, speed, 8);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        copyVec3(&vec, &self->target);
        vec.y += 60.0f;
        vec3_sub_assign(&vec, &self->pos);
        if (vec3_length_sq((const f32*)&vec) > 0.001f) {
            vec3_normalize_into(&vec, &vec);
            vec3_scale(&out, &vec, 8.0f);
            copyVec3(&self->vec_0x310, &out);
        }
        self->timer_0x020 = 240;
        break;
    case 1:
        em_turn_to_target((struct _ENEMY_WORK*)self, 0x1000);
        if (self->field_0x006 == 0 && em_approach_step((struct _ENEMY_WORK*)self, 0, 0) == 1) {
            self->field_0x006 = 1;
        }
        gap = (60.0f + self->target.y) - self->pos.y;
        if (ef_fabsf(gap) < 50.0f) {
            self->field_0x007 = 1;
        } else if (gap < 0.0f) {
            self->pos.y -= 1.0f;
        } else {
            self->pos.y += 1.0f;
        }
        if ((self->field_0x006 == 1 && self->field_0x007 == 1) || self->timer_0x020 <= 0) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
            self->timer_0x020--;
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B8A40 (0x88): action 3's sub-state dispatcher. */
void em036_act3_dispatch(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em036_act3_0(self, 0);
        break;
    case 1:
        em036_act3_1(self, 0);
        break;
    case 2:
        em036_act3_2(self);
        break;
    case 3:
        em036_act3_3(self);
        break;
    case 4:
        em036_act3_4(self, 0);
        break;
    case 5:
        em036_act3_4(self, 1);
        break;
    case 6:
        em036_act3_6(self);
        break;
    case 7:
        em036_act3_0(self, 1);
        break;
    case 8:
        em036_act3_1(self, 1);
        break;
    case 9:
        em036_act3_9(self);
        break;
    case 10:
        em036_act3_10(self);
        break;
    case 11:
        em036_act3_11(self);
        break;
    case 12:
        em036_act3_12(self);
        break;
    case 13:
        em036_act3_13(self);
        break;
    case 15:
        em036_act3_15(self, 0);
        break;
    case 16:
        em036_act3_15(self, 1);
        break;
    }
}

/* 0x801B8AC8 (0xCC): the swing: the 10/8/0 window with the hit window armed, the swing curves applied through
 * the rotation until the motion ends. */
void em036_act7_0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 10, 8, 0);
        em_hit_window_set_default((struct _ENEMY_WORK*)self, 0, 1);
        break;
    case 1:
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.y = em_key_curve_eval((struct _ENEMY_WORK*)self, em036_swing_y_curve);
        self->vec_0x310.z = em_key_curve_eval((struct _ENEMY_WORK*)self, em036_swing_z_curve);
        em_move_offset_rot_apply((struct _ENEMY_WORK*)self, &self->field_0x1BC);
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B8B94 (0xDC): the lunge: the 11/4/0 window with hit window 2 (3 in mode 1) and a 2-unit forward vector. */
void em036_act7_1(EmProgWork* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 11, 4, 0);
        em_hit_window_set_default((struct _ENEMY_WORK*)self, 0, (mode == 1) + 2);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        self->vec_0x310.z = 2.0f;
        rotVecY(&self->vec_0x310, self->field_0x1C0);
        break;
    case 1:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
        }
        break;
    }
    em036_floor_clamp(self);
}

/* 0x801B8C70 (0x1F8): the shot: the 14/2/0 window, a turn at frames 20..100, the shell (kind 16, or 21..23 by mode)
 * with its muzzle effect at frame 126, then the 15 and 16 windows. */
void em036_act7_3(EmProgWork* self, u8 mode) {
    VEC3 pos;
    EmSpawnRec rec;
    u32 kind;

    em_spawn_rec_init(&rec);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 14, 2, 0);
        break;
    case 1:
        if (em_frame_check((struct _ENEMY_WORK*)self, 3, 20.0f, 100.0f) == 1) {
            em_turn_to_target((struct _ENEMY_WORK*)self, 0x200);
        }
        if (em_frame_check((struct _ENEMY_WORK*)self, 0, 126.0f, 0.0f) == 1) {
            rec.id = 24;
            copyVec3(&rec.pos, setVec3(&pos, 0.0f, -25.0f, -20.0f));
            rec.field_0x10 = 0;
            rec.field_0x12 = 0;
            rec.field_0x14 = 0;
            switch (mode) {
            default:
                kind = 16;
                break;
            case 1:
                kind = 21;
                break;
            case 2:
                kind = 22;
                break;
            case 3:
                kind = 23;
                break;
            }
            eft007_part_spawn((struct _ENEMY_WORK*)self, 31, rec.field_0x12, rec.field_0x14);
            ((ShellRecJobFn)shell_set_func_ptr->method_0x3C)((struct _ENEMY_WORK*)self, &rec, kind);
        }
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            em_mot_set((struct _ENEMY_WORK*)self, 15, 0, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            self->state++;
            em_mot_set((struct _ENEMY_WORK*)self, 16, 0, 0);
        }
        break;
    case 3:
        if (em_mot_end_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B8E68 (0x78): action 7's sub-state dispatcher. */
void em036_act7_dispatch(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em036_act7_0(self);
        break;
    case 1:
        em036_act7_1(self, 0);
        break;
    case 2:
        em036_act7_1(self, 1);
        break;
    case 3:
        em036_act7_3(self, 0);
        break;
    case 4:
        em036_act7_3(self, 1);
        break;
    case 5:
        em036_act7_3(self, 2);
        break;
    case 6:
        em036_act7_3(self, 3);
        break;
    }
}

/* The per-sub-state sound programs the two sound players run (`.data`; the name carries the first sub-state that
 * plays each).  The two-float shift the kind-2 records point at (`.sdata`). */
EmSeOffset em036_se_shift = {-5.0f, -1.0f};

extern EmSeBranch em036_se_branch_00;

EmSeRecord em036_se_rec_05 = {0x64, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_seq_05[2] = {
    {0x00, {0, 0, 0}, &em036_se_rec_05},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_00_a = {0x69, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_00_b = {0x67, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_seq_00[2] = {
    {0x09, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em036_se_branch_00)},
    {0x00, {0, 0, 0}, &em036_se_rec_00_a},
};
__declspec(section ".data") EmSeEntry em036_se_seq_00_end[1] = {
    {0xFF, {0, 0, 0}, 0},
};
EmSeEntry em036_se_seq_00_sub[2] = {
    {0x00, {0, 0, 0}, &em036_se_rec_00_b},
    {0x08, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(&em036_se_seq_00_end)},
};
EmSeBranch em036_se_branch_00 = {0, 0x67, em036_se_seq_00_sub};
EmSeRecord em036_se_rec_82_a = {0x65, 0, 2, 0, 0, {0, 2, 1, 0}, &em036_se_shift};
EmSeRecord em036_se_rec_82_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_82_c = {0x67, 0, 2, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em036_se_seq_82[5] = {
    {0x00, {0, 0, 0}, &em036_se_rec_82_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em036_se_rec_82_b},
    {0x00, {0, 0, 0}, &em036_se_rec_82_c},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_80_a = {0x65, 0, 2, 0, 0, {0, 2, 1, 0}, &em036_se_shift};
EmSeRecord em036_se_rec_80_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_seq_80[4] = {
    {0x00, {0, 0, 0}, &em036_se_rec_80_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em036_se_rec_80_b},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_7C_a = {0x6A, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_7C_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_7C_c = {0x67, 0, 2, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em036_se_seq_7C[4] = {
    {0x00, {0, 0, 0}, &em036_se_rec_7C_a},
    {0x00, {0, 0, 0}, &em036_se_rec_7C_b},
    {0x00, {0, 0, 0}, &em036_se_rec_7C_c},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_7A_a = {0x6A, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_7A_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_seq_7A[3] = {
    {0x00, {0, 0, 0}, &em036_se_rec_7A_a},
    {0x00, {0, 0, 0}, &em036_se_rec_7A_b},
    {0xFD, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_7B = {0x67, 0, 2, 0, 0, {1, 0, 0, 0}, 0};
EmSeEntry em036_se_seq_7B[2] = {
    {0x00, {0, 0, 0}, &em036_se_rec_7B},
    {0xFE, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_A9_a = {0x65, 0, 2, 0, 0, {0, 2, 1, 0}, &em036_se_shift};
EmSeRecord em036_se_rec_A9_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_A9_c = {0x67, 0, 2, 0, 0, {4, 0, 0, 0}, 0};
EmSeEntry em036_se_seq_A9[5] = {
    {0x00, {0, 0, 0}, &em036_se_rec_A9_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em036_se_rec_A9_b},
    {0x00, {0, 0, 0}, &em036_se_rec_A9_c},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_A8_a = {0x6A, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_A8_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_rec_A8_c = {0x67, 0, 2, 0, 0, {4, 0, 0, 0}, 0};
EmSeEntry em036_se_seq_A8[4] = {
    {0x00, {0, 0, 0}, &em036_se_rec_A8_a},
    {0x00, {0, 0, 0}, &em036_se_rec_A8_b},
    {0x00, {0, 0, 0}, &em036_se_rec_A8_c},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_8D = {0x6B, 0, 4, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_seq_8D[2] = {
    {0x00, {0, 0, 0}, &em036_se_rec_8D},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_rec_D7 = {0x03, 0, 20, 0, 0, {0, 4, 2, 0}, (const EmSeOffset*)&em036_vec_pair.pos};
EmSeEntry em036_se_seq_D7[3] = {
    {0x03, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0xA)},
    {0x00, {0, 0, 0}, &em036_se_rec_D7},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_alt_rec_00 = {0x68, 0, 0, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_alt_seq_00[3] = {
    {0x0D, {0, 0, 0}, reinterpret_cast<const EmSeRecord*>(0x52)},
    {0x00, {0, 0, 0}, &em036_se_alt_rec_00},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_alt_rec_1D_a = {0x6A, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_alt_rec_1D_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_alt_rec_1D_c = {0x67, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_alt_seq_1D[4] = {
    {0x00, {0, 0, 0}, &em036_se_alt_rec_1D_a},
    {0x00, {0, 0, 0}, &em036_se_alt_rec_1D_b},
    {0x00, {0, 0, 0}, &em036_se_alt_rec_1D_c},
    {0xFF, {0, 0, 0}, 0},
};
EmSeRecord em036_se_alt_rec_1E_a = {0x65, 0, 2, 0, 0, {0, 2, 1, 0}, &em036_se_shift};
EmSeRecord em036_se_alt_rec_1E_b = {0x66, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeRecord em036_se_alt_rec_1E_c = {0x67, 0, 2, 0, 0, {0, 1, 0, 0}, 0};
EmSeEntry em036_se_alt_seq_1E[5] = {
    {0x00, {0, 0, 0}, &em036_se_alt_rec_1E_a},
    {0x04, {0, 0, 0}, 0},
    {0x00, {0, 0, 0}, &em036_se_alt_rec_1E_b},
    {0x00, {0, 0, 0}, &em036_se_alt_rec_1E_c},
    {0xFF, {0, 0, 0}, 0},
};

/* 0x801B8EE0 (0x270): action 10: runs the sound program of the sub-state (unknown sub-states finish the action). */
void em036_act10_se_play(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_00, 0, 0);
        break;
    case 23:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_00, 0, 23);
        break;
    case 5:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_05, 1, 5);
        break;
    case 35:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_05, 1, 35);
        break;
    case 124:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_7C, 0, 124);
        break;
    case 130:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_82, 1, 130);
        break;
    case 141:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_8D, 0, 141);
        break;
    case 125:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_7C, 0, 125);
        break;
    case 131:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_82, 1, 131);
        break;
    case 142:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_8D, 0, 142);
        break;
    case 168:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_A8, 0, 168);
        break;
    case 169:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_A9, 1, 169);
        break;
    case 122:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_7A, 0, 122);
        break;
    case 123:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_7B, 0, 123);
        break;
    case 128:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_80, 1, 128);
        break;
    case 129:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_7B, 0, 129);
        break;
    case 159:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_8D, 0, 159);
        break;
    case 160:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_8D, 0, 160);
        break;
    case 214:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_00, 0, 214);
        break;
    case 215:
        em_se_tbl_play((struct _ENEMY_WORK*)self, em036_se_seq_D7, 1, 215);
        break;
    default:
        em_action_finish((struct _ENEMY_WORK*)self);
        break;
    }
}

/* 0x801B9150 (0xC4): action 11: runs the second player's sound program of the sub-state. */
void em036_act11_se_play(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_00, 0, 0);
        break;
    case 5:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_00, 1, 5);
        break;
    case 29:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_1D, 0, 29);
        break;
    case 30:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_1E, 1, 30);
        break;
    case 20:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_00, 0, 20);
        break;
    case 21:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_00, 1, 21);
        break;
    default:
        em_se_tbl_play_alt((struct _ENEMY_WORK*)self, em036_se_alt_seq_00, 0, 0);
        break;
    }
}

/* 0x801B9214 (0xA8): the drop-in: 300 units up, then the dive until the ground is reached. */
void em036_act12_0(EmProgWork* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 1, 4, 0);
        self->pos.y += 300.0f;
        em_dive_start((struct _ENEMY_WORK*)self);
        break;
    case 1:
        em_dive_step((struct _ENEMY_WORK*)self);
        em_fall_height_get((struct _ENEMY_WORK*)self);
        if (em_ground_ck((struct _ENEMY_WORK*)self) == 1) {
            em_action_finish_fall((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B92BC (0x170): the return run: placed 300 up and 50 behind `aim`, it runs toward the target at 4 units a
 * frame until it is within 70 of `aim`, then hands over to state 3/9. */
void em036_act12_1(EmProgWork* self) {
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 sum;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 out;

    VEC3_ctor(&ofs);
    VEC3_ctor(&dir);
    switch (self->state) {
    case 0:
        self->state++;
        em_fall_height_get((struct _ENEMY_WORK*)self);
        em_fall_start((struct _ENEMY_WORK*)self);
        em_mot_set((struct _ENEMY_WORK*)self, 1, 0, 0);
        setVector3(&ofs, 0.0f, 300.0f, -50.0f);
        rotVecY(&ofs, self->field_0x1C0);
        addVec3(&sum, &self->aim, &ofs);
        copyVec3(&self->pos, &sum);
        em_reaction_req((struct _ENEMY_WORK*)self, 5, 6, 0);
        em_target_pos_set((struct _ENEMY_WORK*)self, 0);
        em_move_vec2_clr((struct _ENEMY_WORK*)self);
        subVec3(&diff, &self->target, &self->pos);
        copyVec3(&dir, &diff);
        if (vec3_length_sq((const f32*)&dir) > 0.001f) {
            vec3_normalize_into(&dir, &dir);
            vec3_scale(&out, &dir, 4.0f);
            copyVec3(&self->vec_0x310, &out);
        }
        break;
    case 1:
        if (vec3_distance(&self->pos, &self->aim) < 70.0f) {
            em_state_set((struct _ENEMY_WORK*)self, 3, 9);
        } else {
            em_move_offset_apply((struct _ENEMY_WORK*)self);
        }
        break;
    }
}

/* 0x801B942C (0x24): action 12's sub-state dispatcher. */
void em036_act12_dispatch(EmProgWork* self) {
    switch (self->state_sub) {
    case 0:
        em036_act12_0(self);
        break;
    case 1:
        em036_act12_1(self);
        break;
    }
}

/* 0x801B9450 (0x48): the action dispatcher. */
void em036_action_dispatch(EmProgWork* self) {
    switch (self->action) {
    case 0:
        em036_act0_dispatch(self);
        break;
    case 1:
        em036_act1_dispatch(self);
        break;
    case 2:
        em036_act2_dispatch(self);
        break;
    case 3:
        em036_act3_dispatch(self);
        break;
    case 7:
        em036_act7_dispatch(self);
        break;
    case 10:
        em036_act10_se_play(self);
        break;
    case 11:
        em036_act11_se_play(self);
        break;
    case 12:
        em036_act12_dispatch(self);
        break;
    }
}

/* 0x801B9498 (0xD0): sets the motion-event flags from the motion number: the moving motions toggle bit 0 (and set
 * bit 2 when it was set), every other motion sets bit 0 alone. */
void em036_motion_events(EmProgWork* self) {
    switch (em_get_mot_no((struct _ENEMY_WORK*)self)) {
    case 1:
    case 2:
    case 3:
    case 5:
    case 10:
    case 11:
    case 14:
    case 15:
    case 16:
    case 100:
        if (self->field_0x761 & 1) {
            self->field_0x761 &= ~1;
            self->field_0x761 &= ~2;
            self->field_0x761 |= 4;
        } else {
            self->field_0x761 |= 1;
            self->field_0x761 |= 2;
            self->field_0x761 &= ~4;
        }
        break;
    default:
        self->field_0x761 |= 1;
        self->field_0x761 &= ~2;
        self->field_0x761 &= ~4;
        break;
    }
}

/* 0x801B9568 (0x154): fades the tint level toward 150 while the program reports busy (toward 255 otherwise) and
 * scales the model's material colour by it. */
void em036_tint_update(EmProgWork* self) {
    GXColor color;
    f32 scale;

    if (em034_eft_ck((struct _ENEMY_WORK*)self) == 1) {
        self->tint_0x330 -= 5;
        if (self->tint_0x330 < 150) {
            self->tint_0x330 = 150;
        }
    } else {
        self->tint_0x330 += 5;
        if (self->tint_0x330 > 255) {
            self->tint_0x330 = 255;
        }
    }
    if (((MHchar*)&self->char_0x024)->getMatColor(0, GX_COLOR0A0, &color) != 0) {
        scale = (f32)self->tint_0x330 / 255.0f;
        color.r = (f32)color.r * scale;
        color.g = (f32)color.g * scale;
        color.b = (f32)color.b * scale;
        ((MHchar*)&self->char_0x024)->setMatColor(0, GX_COLOR0A0, color, false);
    }
}

/* 0x801B96BC (0x8): the state query; em036 reports nothing. */
s32 em036_state_ck(void) {
    return 0;
}

/* 0x801B96C4 (0xC4): the area start: action 12, sub-state 1 for the random motion (0 otherwise), the motion
 * parameters reset and the work placed on its ground record. */
void em036_area_start_set(EmProgWork* self, u8* action, u8* sub) {
    EmSelRec rec;

    em_ground_rec_clear(&rec);
    em_fall_height_get((struct _ENEMY_WORK*)self);
    em_fall_start((struct _ENEMY_WORK*)self);
    *action = 12;
    if (self->field_0x00A & 1) {
        *sub = 1;
    } else {
        *sub = 0;
    }
    em_motion_param_set((struct _ENEMY_WORK*)self, 0.0f, 0);
    em_motion_param_set((struct _ENEMY_WORK*)self, 1.0f, 30);
    if (em_ground_rec_find(self->field_0x01A, &rec) == 1) {
        copyVec3(&self->pos, &rec.vec_0x08);
    }
}

/* 0x801B9788 (0x4): the gauge slot; em036 has none. */
void em036_gauge_update(void) {
}

/* 0x801B978C (0x14): whether the state is clear (`field_0x1E2` is 1). */
s32 em036_state_clear_ck(EmProgWork* self) {
    return self->field_0x1E2 == 1;
}

/* 0x801B97A0 (0xBC): rolls the random kind into an area record's team bit (modes 1 and 4 always, the others only
 * for a record that was never rebuilt). */
void em036_mini_init(_ENEMY_MINI_WORK* mini, u8 mode) {
    s32 pick;

    if ((s32)(u8)mode == 1 || (s32)(u8)mode == 4) {
        pick = (s8)em036_ground_variant_roll(mini->order_0x16);
        if (pick == 1) {
            mini->team_0x03 &= 0xFD;
        } else if (pick == 2) {
            mini->team_0x03 |= 0x02;
        }
    } else if (mini->field_0x10 == 0) {
        pick = (s8)em036_ground_variant_roll(mini->order_0x16);
        if (pick == 1) {
            mini->team_0x03 &= 0xFD;
        } else if (pick == 2) {
            mini->team_0x03 |= 0x02;
        }
    }
}

}

/* The unit's `.bss` (0x806A7AB8-0x806A7AD0): the vector pair MWCC's `__sinit` seeds (a point 40 units ahead and a
 * zero step); the action-10 sub-state 215 sound record points at it. */
Em036VecPair em036_vec_pair(-40.0f);
