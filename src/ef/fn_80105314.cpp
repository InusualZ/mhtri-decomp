/* ef/fn_80105314.cpp - the `_EFT` enemy/emitter base cluster: per-frame handlers, state dispatchers, pool releases,
 *   type dispatch tables and the setters, ahead of the eft013 families in `ef/eft013_fx.cpp`.
 * RANGE. .text 0x80105314-0x80107250 (35 functions); extab 0x8000BFBC-0x8000C074, extabindex 0x80025E84-0x80025F98,
 *   .data 0x8059E688-0x8059EB50, .sdata2 0x80796784-0x807967A8.  The seam is unproven (a discovery cut).
 * FLAGS. `cflags_main`; file-scope `#pragma peephole off` and `optimization_level 4` (retail's switch lowering; the
 *   O4 move lifted seven rows and lowered none once the sources were fixed).
 * NAMES. The map has only `fn_` stems here apart from `eft_spawn_type10` and `eft_spawn_type11`, GUESSes from their
 *   bodies; plain definitions are `extern "C"` in a C++ unit.
 *   GUESS (from what they hold and their callers): `eft013_em_effect_ids`, `eft013_em_param_ids`, `eft013_spawn_offsets`
 *   and the callees `ef_effect_root_mtx`, `em_eft_active_ck`, `em018_glow_get`; the declaration of `setScaleMat34W`
 *   is a STOPGAP (ef-game3#78).
 * RESIDUALS. 2 partial rows: `fn_801059E8` and `fn_801063A8` assign their callee-saved registers in another order (the
 *   structure and every operand but the register numbers match; no declaration order reproduces retail's).
 *   `.data` emitted (the late tables defined before `fn_8010695C`, after the first jump tables, as retail places them);
 *   `.sdata2` short: retail keeps nine words (0.0 four times, 0.5 and 1.0 twice) where our literals pool four.
 *   flipcheck: NOT READY on `.sdata2` alone (0x10 of 0x24, the pool above).
 * SHAPES. `eft_spawn_type10` takes its type as `u8` (retail stores it raw) while `ef/fn_80105314.h` keeps the `u32`
 *   its consumers use (`enemy/em016_prog.cpp` has a local `u32` copy), so the header's declaration is hidden here.
 *   `fn_80106DC8` and `eft013_shell_spawn` take the float before the trailing pointer (retail saves `f1` before it).
 *   `em_water_check`/`em_die_ck` are called through `u32`-returning views (`cmplwi` in retail).
 *   Definitions follow retail's address order.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "ef/eft004.h"
/* the header spells `eft_spawn_type10`'s type `u32` for its consumers; the definition below takes it as `u8` (retail stores
 * it raw), so the header's declaration is kept out of this TU. */
#define eft_spawn_type10 eft_spawn_type10_hidden_by_header
#include "ef/fn_80105314.h"
#undef eft_spawn_type10
#include "ef/eft_res.h"
#include "ef/fn_8010BDE4.h"
#include "gx.h"
#include "pl.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_80105314_types.h"
#include "camera/camera.h" /* get_camera_pos (rule 2) */
#include "enemy/fn_8012BDF4.h" /* em_work_die_ck (rule 2) */
#include "fn_8004CAD8.h" /* addVec3, addVec3To, vec_to_mh_vec3, vec3_scale_in_place (rule 2) */
#include "ef/mtx34_trans_get.h"
#include "ef/mtx34_trans_add.h"
#include "ef/eft001.h" /* mtx34_set_trans */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */
#include "enemy/fn_8012EC74.h" /* em_water_check, get_em_scale, get_joint_wmat_em (rule 2) */
#include "rotLocalMatX.h"
#include "g3d/g3d_scnroot.h" /* VEC2_ctor (rule 2) */
#include "ef/RetireParticleAll.h"
#include "menu/get_pop_dat_ptr.h" /* get_option_21 (rule 2) */
#include "ef/ef_effect_root_mtx.h" /* ef_effect_root_mtx (rule 2: a leaf header) */
#include "enemy/em018_glow_get.h" /* em018_glow_get (rule 2: a leaf header) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call
 * (both return the flag in r3, compared unsigned). */
#define em_water_check_c1 ((u32 (*)(_ENEMY_WORK*))em_water_check)
#define em_die_ck_c1 ((u32 (*)(_ENEMY_WORK*))em_die_ck)

/* size: 0x10 - lower bound, an approximation (the pooled block the handlers walk). */

/* Pool block of the `fn_80106EE4` family: the effect array sits at +0x0C. */
struct _EFT013_POOL2 {
    /* +0x00 */ s32 count;
    /* +0x04 */ f32 scale_0x04;
    /* +0x08 */ f32 scale_0x08;
    /* +0x0C */ nw4r::ef::Effect* effects[1];
};
/* size: 0x10 - lower bound, an approximation. */

/* The spawner source `fn_80106DC8` reads: the area byte, a pointer and a flag. */
struct _EFT013_SRC {
    /* +0x00 */ u8 unused_0x00[0x0F];
    /* +0x0F */ u8 area_0x0F;
    /* +0x10 */ u8 unused_0x10[0x2C - 0x10];
    /* +0x2C */ void* field_0x2C;
    /* +0x30 */ u8 field_0x30;
};
/* size: 0x34 - lower bound, an approximation (only +0x0F, +0x2C and +0x30 are read). */

/* Pool block of the `eft_spawn_type10` / `fn_80105888` family: the type-table value at +0x00, the caller's
 * first word at +0x10, the position copy at +0x14 and the scale at +0x20. */
struct _EFT013_WORK_A {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[3];
    /* +0x10 */ u32 joint_0x10;  /* the enemy joint the effects follow (the caller's first word) */
    /* +0x14 */ nw4r::math::VEC3 pos_0x14;
    /* +0x20 */ f32 scale_0x20;
};
/* size: 0x24 - lower bound, an approximation. */

/* ---------------------------------------------------------------------------------------------------
 * externs - mangled callees are declared through their real signature (rule 9)
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_80050708(void* mtx, nw4r::math::VEC3* v);

/* ---------------------------------------------------------------------------------------------------
 * the unit's tables (.data), in address order
 * ------------------------------------------------------------------------------------------------- */

/* The number of pooled effects each enemy-effect type (0..32) creates. */
u8 eft013_effect_counts[40] = {
    2, 1, 1, 1, 2, 1, 3, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 3, 1, 1, 1, 1, 1, 1,
};

/* The joint offsets of the types that place their effects apart (one x, y, z triple per pooled effect). */
f32 eft013_offsets_0[6] = {10.0f, 0.0f, 50.0f, -10.0f, 0.0f, 50.0f};
f32 eft013_offsets_8[6] = {80.0f, 0.0f, 0.0f, -100.0f, 0.0f, 0.0f};
f32 eft013_offsets_6[10] = {0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 50.0f, -10.0f, 0.0f, 50.0f};
f32 eft013_offsets_9[6] = {40.0f, -10.0f, 70.0f, -40.0f, -10.0f, 70.0f};
f32 eft013_offsets_10[3] = {0.0f, -10.0f, 30.0f};
f32 eft013_offsets_15[3] = {0.0f, 0.0f, 20.0f};
f32 eft013_offsets_16[4] = {0.0f, -70.0f, -40.0f};
f32 eft013_offsets_24[6] = {-30.0f, 0.0f, 0.0f, 30.0f, 0.0f, 0.0f};
f32 eft013_offsets_25[9] = {20.0f, 0.0f, 40.0f, -20.0f, 0.0f, 40.0f};
f32 eft013_offsets_26[9] = {10.0f, 0.0f, 10.0f, -10.0f, 0.0f, 10.0f};
Vec* eft013_spawn_offsets[33] = {
    (Vec*)eft013_offsets_0, NULL, NULL, NULL, (Vec*)eft013_offsets_0, NULL, (Vec*)eft013_offsets_6, NULL,
    (Vec*)eft013_offsets_8, (Vec*)eft013_offsets_9, (Vec*)eft013_offsets_10, NULL, NULL, NULL, NULL,
    (Vec*)eft013_offsets_15, (Vec*)eft013_offsets_16, (Vec*)eft013_offsets_15, (Vec*)eft013_offsets_16, NULL, NULL,
    NULL, NULL, NULL, (Vec*)eft013_offsets_24, (Vec*)eft013_offsets_25, (Vec*)eft013_offsets_26,
};

/* The effect and parameter ids of each enemy-effect type. */
u16 eft013_em_effect_ids[34] = {
    0x0109, 0x0136, 0x0107, 0x01AB, 0x010B, 0x01A9, 0x010D, 0x00D4, 0x020C, 0x023A, 0x023B, 0x0136,
    0x03B5, 0x0560, 0x05E3, 0x05F3, 0x05F4, 0x05F5, 0x05F6, 0x06AE, 0x06AD, 0x06B3, 0x06B4, 0x08A3,
    0x08F3, 0x08F5, 0x09F5, 0x0A0C, 0x0A10, 0x0A11, 0x0A27, 0x0A28, 0x0A37,
};
u16 eft013_em_param_ids[34] = {
    0x0003, 0x0022, 0x0022, 0x0024, 0x0003, 0x0022, 0x0003, 0x0022, 0x0033, 0x0026, 0x0026, 0x0022,
    0x0045, 0x0043, 0x0042, 0x0045, 0x0045, 0x0045, 0x0045, 0x005E, 0x005E, 0x0072, 0x0072, 0x0043,
    0x005B, 0x0071, 0x0062, 0x00A1, 0x004C, 0x004C, 0x0070, 0x0070, 0x0045,
};
/* The tables of the second spawner pair, defined further down (after the first families' jump tables, where retail
 * emits them). */
extern u16 eft013_burst_ids[8];
extern u16 eft013_burst_params[8];
extern u16 eft013_shell_ids[8];
extern u16 eft013_shell_params[8];
extern u8 eft013_shell_colors[20];
extern u8 eft013_shell_colors_alt[20];



/* STOPGAP-BEGIN(ef-game3#78) */
void setScaleMat34W(nw4r::math::MTX34* mtx, nw4r::math::VEC3* scale);
/* STOPGAP-END(ef-game3#78) */

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
/* `se_req_pos_ps` comes from the owner's header `sound/fn_800D7F54.h` (rule 2); this unit's local
 * `void` copy collided with the owner's `SeSlot*` once the header declared it. */
u8 get_now_areano();
u32 em_sleep_ck(_ENEMY_WORK* enemy, u8 kind);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* mtx);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
u32 event_demo_ck();

extern "C" _EFT013* fn_80105888(_ENEMY_WORK* self, u8 type, u32 a, nw4r::math::VEC3* b, f32 scale);

/* ---------------------------------------------------------------------------------------------------
 * state dispatchers and pool releases
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_801059E8(_EFT013* self);
extern "C" void fn_80105EFC(_EFT013* self);
extern "C" void fn_80105F30(_EFT013* self);
extern "C" void fn_801065FC(_EFT013* self, _ENEMY_WORK* enemy, nw4r::math::VEC3* v, u32 joint, u32 index);
extern "C" void fn_80106180(_EFT013* self);
extern "C" void fn_801063A8(_EFT013* self);
extern "C" void fn_801068E4(_EFT013* self);
extern "C" void fn_80106920(_EFT013* self);
extern "C" void fn_80106530(_EFT013* self);
extern "C" void fn_801065F8(_EFT013* self);
extern "C" void fn_8010695C(_EFT013* self);
extern "C" void fn_80106ACC(_EFT013* self);
extern "C" void fn_80106BA0(_EFT013* self);
extern "C" void fn_80106BB0(_EFT013* self);
extern "C" void fn_80106F5C(_EFT013* self);
extern "C" void fn_8010710C(_EFT013* self);
extern "C" void fn_8010723C(_EFT013* self);
extern "C" void fn_8010724C(_EFT013* self);
extern "C" void fn_80105970(_EFT013* self);
extern "C" void fn_801059AC(_EFT013* self);
extern "C" void fn_80106EE4(_EFT013* self);
extern "C" void fn_80106F20(_EFT013* self);

#pragma peephole off
#pragma optimization_level 4
#pragma pool_data off

/* The shell `_EFT013_SHELL_SRC::shell_0x2C` points at: its type picks the colour set. size: 0x0B - lower bound */
struct _EFT013_SHELL {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 type_0x03;
    /* +0x04 */ u8 unused_0x04[0x06];
    /* +0x0A */ u8 level_0x0A;      /* 3..4 select colour set 2 for type 32 */
};

/* The actor record `eft013_shell_spawn` spawns for: the area byte, the equipped shell and its kind.  The same view as
 * `ef/eft002.cpp`'s `_EFT001_SRC`. size: 0x31 - lower bound, an approximation */
struct _EFT013_SHELL_SRC {
    /* +0x00 */ u8 unused_0x00[0x06];
    /* +0x06 */ u8 field_0x06;     /* with kind 3, zero refuses the first types */
    /* +0x07 */ u8 unused_0x07[0x08];
    /* +0x0F */ u8 area_0x0F;
    /* +0x10 */ u8 unused_0x10[0x1C];
    /* +0x2C */ struct _EFT013_SHELL* shell_0x2C; /* the equipped shell */
    /* +0x30 */ u8 kind_0x30;      /* 1 = a shell is equipped, 3 = the bare kind */
};

/* The pool block `fn_80105314` drives: the pooled effects from +0x08 and the colour it hands change_color_eff.
 * size: 0x14 - lower bound, an approximation. */
struct _EFT013_COLOR_POOL {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ nw4r::ef::Effect* effects[2];
    /* +0x10 */ _GXColor color_0x10;
};

/* State 1 of the enemy-effect controller: ends when the area changes or a demo-only effect leaves the demo, moves
 * the pooled effects (ending when all stopped), recolours them per type (types 44/45/121 only while the camera is
 * at or above y 0, type 120 only at or below it; the listed types keep their colour) and hands them on. */
/* untyped: a caller-owned payload (the effect record eft009 and fn_80104BD0 pass under their own views) */
extern "C" void eft_em_effects_move(void* self_)
{
    _EFT013* self = (_EFT013*)self_;
    s32 i;
    u8 stopped;
    _EFT013_COLOR_POOL* work;

    stopped = 0;
    work = (_EFT013_COLOR_POOL*)self->work_0x38;
    if (self->area_0x44 != (u8)get_now_areano()) {
        self->state_0x05++;
        return;
    }
    if (self->field_0x08 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    for (i = 0; i < work->count; i++) {
        if (effect_move(work->effects[i]) == 0) {
            stopped++;
        }
    }
    if (stopped == work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    for (i = 0; i < work->count; i++) {
        switch (self->type_0x02) {
        case 44:
        case 45:
        case 121:
            if (get_camera_pos().y < 0.0f) {
                return;
            }
            change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x10);
            break;
        case 120:
            if (get_camera_pos().y > 0.0f) {
                return;
            }
            change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x10);
            break;
        case 73:
        case 76: case 77:
        case 88: case 89: case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97: case 98: case 99:
        case 100: case 101: case 102: case 103: case 104: case 105:
        case 107: case 108: case 109: case 110: case 111:
        case 125: case 126: case 127: case 128:
        case 145:
            break;
        default:
            change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x10);
            break;
        }
    }
    eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, NULL);
}

/* 0x80105550 - the trivial state advance (`state_0x05++`). */
extern "C" void fn_80105550(void* self) {
    ((_EFT013*)self)->state_0x05++;
}

/* 0x80105560 - destroy the effect object. */
extern "C" void fn_80105560(void* self) {
    eft_res_slot_release(self);
}

/* 0x80105564 - per-type sound/position dispatch.  The comparison tree is the compiler's binary search
 * over the case values, so the case list is read off it (see the runs below). */
extern "C" void fn_80105564(_EFT013* self) {
    switch (self->type_0x02) {
    case 12:
    case 13:
    case 19:
    case 20:
        fn_800DC60C(&self->pos_0x18, 1);
        return;
    case 16:
    case 17:
    case 18:
    case 21:
    case 45:
        fn_800DC60C(&self->pos_0x18, 2);
        return;
    case 22:
    case 24:
    case 25:
    case 53:
        fn_800DC60C(&self->pos_0x18, 0);
        return;
    case 44:
        fn_800DB964(&self->pos_0x18);
        return;
    case 76:
        se_req_pos_ps(self->source_0x30->se_handle_0xB14, 157, 2, &self->pos_0x18);
        return;
    default:
        return;
    }
}

/* 0x8010562C (0x178): Spawns an enemy effect gated on the area and the enemy's sleep state, copying the position
 * into `pos_0x18` or the work block's `pos_0x14` by type. */
extern "C" void eft_spawn_type10(_ENEMY_WORK* self, u8 type, u32 a, nw4r::math::VEC3* b, f32 scale) {
    _EFT013* e = (_EFT013*)eft_res_slot_get(0x24);
    if (e == NULL) {
        return;
    }
    _EFT013_WORK_A* work = (_EFT013_WORK_A*)e->work_0x38;
    work->count = eft013_effect_counts[(u8)type];
    if (work->count > 3) {
        eft_res_slot_release(e);
        return;
    }
    if (self->act_id != get_now_areano()) {
        eft_res_slot_release(e);
        return;
    }
    if (em_sleep_ck(self, 0) == 1) {
        eft_res_slot_release(e);
        return;
    }
    switch ((u8)type) {
    case 3:
    case 5:
    case 7:
    case 9:
    case 10:
    case 11:
    case 15:
    case 16:
    case 17:
    case 18:
    case 28:
    case 29:
    case 30:
    case 31:
        copyVec3(&e->pos_0x18, b);
        break;
    default:
        copyVec3(&work->pos_0x14, b);
        break;
    }
    work->joint_0x10 = a;
    work->scale_0x20 = scale;
    e->field_0x03 = 10;
    e->type_0x02 = type;
    e->source_0x30 = self;
    e->area_0x44 = self->act_id;
    e->rot_0x24.x = self->pos_0x1BC.x;
    e->rot_0x24.y = self->pos_0x1BC.y;
    e->rot_0x24.z = self->pos_0x1BC.z;
    e->timer_0x0C = 0;
    e->field_0x10 = 0;
    eft_state_flags_set((_EFT*)e, 0, 0);
    e->release_0x40 = fn_80105970;
    e->dispatch_0x34 = fn_801059AC;
}

/* 0x801057A4 - setter: spawn type 2 and store the trailing id at +0x24. */
extern "C" void fn_801057A4(void* self, u32 a, void* v, f32 scale, u32 id) {
    _EFT013* e = fn_80105888((_ENEMY_WORK*)self, 2, a, (nw4r::math::VEC3*)v, scale);
    if (e != NULL) {
        e->rot_0x24.x = id;
        eft_state_flags_set((_EFT*)e, 1, 0);
    }
}

/* 0x801057FC - setter: spawn the type in the low byte of `type` and store the trailing id at +0x24. */
extern "C" void fn_801057FC(_ENEMY_WORK* self, u32 type, u32 a, nw4r::math::VEC3* b, f32 scale, u32 id) {
    _EFT013* e = fn_80105888(self, (u8)type, a, b, scale);
    if (e != NULL) {
        e->rot_0x24.x = id;
        eft_state_flags_set((_EFT*)e, 1, 0);
    }
}

/* 0x80105844 - setter: spawn type in the low byte with a scale of 1.0 and zero the id. */
extern "C" void fn_80105844(_ENEMY_WORK* self, u32 type, u32 a, nw4r::math::VEC3* b) {
    _EFT013* e = fn_80105888(self, (u8)type, a, b, 1.0f);
    if (e != NULL) {
        e->rot_0x24.x = 0;
        eft_state_flags_set((_EFT*)e, 1, 0);
    }
}

/* 0x80105888 (0xE8): Allocates the 0x24-slot object every `fn_801057xx` setter funnels through, seeds it from the
 * type table and installs the release/dispatch pair. */
extern "C" _EFT013* fn_80105888(_ENEMY_WORK* self, u8 type, u32 a, nw4r::math::VEC3* b, f32 scale) {
    _EFT013* e = (_EFT013*)eft_res_slot_get(0x24);
    if (e == NULL) {
        return NULL;
    }
    _EFT013_WORK_A* work = (_EFT013_WORK_A*)e->work_0x38;
    work->count = eft013_effect_counts[(u8)type];
    work->joint_0x10 = a;
    copyVec3(&work->pos_0x14, b);
    work->scale_0x20 = scale;
    e->field_0x03 = 10;
    e->type_0x02 = type;
    e->source_0x30 = self;
    e->area_0x44 = self->act_id;
    e->rot_0x24.x = self->pos_0x1BC.x;
    e->rot_0x24.y = self->pos_0x1BC.y;
    e->rot_0x24.z = self->pos_0x1BC.z;
    e->timer_0x0C = 0;
    e->field_0x10 = 0;
    e->release_0x40 = fn_80105970;
    e->dispatch_0x34 = fn_801059AC;
    return e;
}

/* 0x80105970 - release the pool block (effects at +0x04, count at +0x00). */
extern "C" void fn_80105970(_EFT013* self) {
    _EFT013_POOL* pool = (_EFT013_POOL*)self->work_0x38;
    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* 0x801059AC - state_0x05 dispatcher for the fn_801059E8 family. */
extern "C" void fn_801059AC(_EFT013* self) {
    switch (self->state_0x05) {
    case 0:
        fn_801059E8(self);
        return;
    case 1:
        fn_80105EFC(self);
        return;
    case 2:
        fn_80106530(self);
        return;
    case 3:
        fn_801065F8(self);
        return;
    default:
        return;
    }
}

/* State 0 of the enemy joint family: creates the type's pooled effects and places them - on the enemy's joint
 * matrix (with the type's offset table), at the work's offset or at the record's position - then runs state 1;
 * type 8 also plays its sound at the midpoint of the effect and the record. */
extern "C" void fn_801059E8(_EFT013* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 scale;
    _EFT013_WORK_A* work;
    _ENEMY_WORK* enemy;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&offset);
    VEC3_ctor(&pos);
    VEC3_ctor(&scale);
    work = (_EFT013_WORK_A*)self->work_0x38;
    enemy = self->source_0x30;
    self->state_0x05++;
    self->flag_0x01 = 1;
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }
    switch (self->type_0x02) {
    case 0:
    case 4:
    case 6:
    case 25:
    case 26:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(i + eft013_em_effect_ids[self->type_0x02],
                                              eft013_em_param_ids[self->type_0x02], 0);
            if (work->effects[i] == NULL) {
                fn_801065F8(self);
                return;
            }
        }
        fn_80105EFC(self);
        break;
    case 1:
    case 14:
    case 19:
    case 21:
    case 22:
    case 23:
    case 27:
        get_joint_wmat_em(enemy, work->joint_0x10, &mtx);
        if ((s32)self->type_0x02 == 23) {
            setVector3(&scale, 1.0f, 1.0f, 1.0f);
            setScaleMat34W(&mtx, &scale);
        }
        copyVec3(&offset, &work->pos_0x14);
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(eft013_em_effect_ids[self->type_0x02], eft013_em_param_ids[self->type_0x02], 0);
            if (work->effects[i] == NULL) {
                fn_801065F8(self);
                return;
            }
            mulVecMat(&offset, &mtx);
            mtx34_trans_add(&mtx, &offset);
            mtx34_trans_get(&mtx, &self->pos_0x18);
            work->effects[i]->SetRootMtx(mtx);
        }
        fn_80105EFC(self);
        break;
    case 2:
    case 8:
    case 12:
    case 13:
    case 20:
    case 32:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(eft013_em_effect_ids[self->type_0x02], eft013_em_param_ids[self->type_0x02], 0);
            if (work->effects[i] == NULL) {
                fn_801065F8(self);
                return;
            }
        }
        fn_80105EFC(self);
        break;
    case 3:
    case 5:
    case 7:
    case 9:
    case 10:
    case 11:
    case 15:
    case 16:
    case 17:
    case 18:
    case 28:
    case 29:
    case 30:
    case 31:
        get_joint_wmat_em(enemy, work->joint_0x10, &mtx);
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(eft013_em_effect_ids[self->type_0x02], eft013_em_param_ids[self->type_0x02], 0);
            if (work->effects[i] == NULL) {
                fn_801065F8(self);
                return;
            }
            if ((u32)(self->type_0x02 - 9) <= 1) {
                work->effects[i]->owner_0x44 = self;
            }
            if (eft013_spawn_offsets[self->type_0x02] != NULL) {
                nw4r::math::VEC3 sum;
                vec_to_mh_vec3(&offset, &eft013_spawn_offsets[self->type_0x02][i]);
                mulVecMat(&offset, &mtx);
                addVec3(&sum, &self->pos_0x18, &offset);
                copyVec3(&pos, &sum);
            } else {
                copyVec3(&pos, &self->pos_0x18);
            }
            mtx34_set_trans(&mtx, &pos);
            mtx34_trans_get(&mtx, &self->pos_0x18);
            work->effects[i]->SetRootMtx(mtx);
        }
        fn_80105EFC(self);
        break;
    case 24:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(i + eft013_em_effect_ids[self->type_0x02],
                                              eft013_em_param_ids[self->type_0x02], 0);
            if (work->effects[i] == NULL) {
                fn_801065F8(self);
                return;
            }
            copyVec3(&offset, &work->pos_0x14);
            if (eft013_spawn_offsets[self->type_0x02] != NULL) {
                Vec* add = &eft013_spawn_offsets[self->type_0x02][i];
                offset.x += add->x;
                offset.y += add->y;
                offset.z += add->z;
            }
            get_joint_wmat_em(enemy, work->joint_0x10, &mtx);
            mulVecMat(&offset, &mtx);
            mtx34_trans_add(&mtx, &offset);
            mtx34_trans_get(&mtx, &self->pos_0x18);
            work->effects[i]->SetRootMtx(mtx);
        }
        fn_80105EFC(self);
        break;
    }
    if ((s32)self->type_0x02 == 8) {
        nw4r::math::MTX34* root = ef_effect_root_mtx(work->effects[0]);
        pos.x = root->m[0][3];
        pos.y = root->m[1][3];
        pos.z = root->m[2][3];
        addVec3To(&pos, &self->pos_0x18);
        vec3_scale_in_place(&pos, 0.5f);
        se_req_pos_ps(enemy->se_handle_0xB14, 22, 2, &pos);
    }
}

/* 0x80105EFC (0x34): Dispatches on the type through a 33-entry jump table (`jumptable_8059E9AC`) with no default
 * body; the non-case values share `fn_80105F30`'s run. */
extern "C" void fn_80105EFC(_EFT013* self) {
    switch (self->type_0x02) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        fn_80105F30(self);
        return;
    case 2:
    case 13:
    case 20:
    case 32:
        fn_80106180(self);
        return;
    case 12:
        fn_801063A8(self);
        return;
    default:
        return;
    }
}

/* State 1 of the enemy joint family: ends with the enemy; the free-standing types re-attach each effect to the
 * enemy joint at the work's offset (plus the type's offset table); then the effects are scaled and moved (ending when
 * one stops), the area follows the enemy and types 3/5/7/17/18 take the stage colour while the camera is at or below
 * y 0 (above it they are not handed on). */
extern "C" void fn_80105F30(_EFT013* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    s32 i;
    u32 spawn;
    _ENEMY_WORK* enemy;
    _EFT013_WORK_A* work;
    u32 col;
    _GXColor color;

    spawn = 1;
    VEC3_ctor(&offset);
    enemy = self->source_0x30;
    work = (_EFT013_WORK_A*)self->work_0x38;
    MTX34_ctor(&mtx);
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    switch (self->type_0x02) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    case 10:
    case 11:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 21:
    case 22:
    case 23:
    case 24:
    case 28:
    case 29:
    case 30:
    case 31:
        break;
    default:
        for (i = 0; i < work->count; i++) {
            if (eft013_spawn_offsets[self->type_0x02] != NULL) {
                vec_to_mh_vec3(&offset, &eft013_spawn_offsets[self->type_0x02][i]);
                addVec3To(&offset, &work->pos_0x14);
            } else {
                copyVec3(&offset, &work->pos_0x14);
            }
            fn_801065FC(self, enemy, &offset, work->joint_0x10, (u8)i);
        }
        break;
    }
    for (i = 0; i < work->count; i++) {
        change_paramscale_eff(work->effects[i], work->scale_0x20);
        if (effect_move(work->effects[i]) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
    }
    self->area_0x44 = enemy->act_id;
    if ((u32)(self->type_0x02 - 17) <= 1 || (s32)self->type_0x02 == 3 || (s32)self->type_0x02 == 5 ||
        (s32)self->type_0x02 == 7) {
        if (get_camera_pos().y > 0.0f) {
            spawn = 0;
        } else {
            col = get_stg_eft_col(self->area_0x44, 1);
            color.r = (col & 0xFF000000) >> 24;
            color.g = (col & 0xFF0000) >> 16;
            color.b = (col & 0xFF00) >> 8;
            color.a = col;
            for (i = 0; i < work->count; i++) {
                change_color_eff(work->effects[i], &self->pos_0x18, color);
            }
        }
    }
    if (spawn == 1) {
        eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, NULL);
    }
}

/* State 1 of the enemy follow family: on the enemy's death runs state 2; while the enemy dies the particles are retired
 * once; otherwise, while the enemy is active, awake and out of the water, the effects are placed on its joint at the
 * work's offset (rotated by the record's x angle, type 13 at unit scale), scaled by the enemy and handed on. */
extern "C" void fn_80106180(_EFT013* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 joint_pos;
    nw4r::math::VEC3 scale;
    nw4r::math::VEC2 unused;
    s32 i;
    _ENEMY_WORK* enemy;
    _EFT013_WORK_A* work;

    enemy = self->source_0x30;
    work = (_EFT013_WORK_A*)self->work_0x38;
    MTX34_ctor(&mtx);
    VEC3_ctor(&offset);
    VEC3_ctor(&joint_pos);
    VEC3_ctor(&scale);
    VEC2_ctor(&unused);
    if (em_work_die_ck(enemy) != 0) {
        self->state_0x05++;
        self->field_0x06 = 0;
        self->field_0x10 = 20;
        fn_80106530(self);
        return;
    }
    if (em_die_ck_c1(enemy) == 1) {
        if (self->field_0x08 == 0) {
            for (i = 0; i < work->count; i++) {
                RetireParticleAll(work->effects[i]);
            }
            self->field_0x08++;
        }
    } else {
        self->field_0x08 = 0;
        if (em_eft_active_ck(enemy) != 0 && em_sleep_ck(enemy, 0) != 1) {
            get_joint_wpos_em(enemy, work->joint_0x10, &joint_pos);
            if (em_water_check_c1(enemy) != 1) {
                self->area_0x44 = enemy->act_id;
                copyVec3(&offset, &work->pos_0x14);
                get_joint_wmat_em(enemy, work->joint_0x10, &mtx);
                mulVecMat(&offset, &mtx);
                mtx34_trans_add(&mtx, &offset);
                mtx34_trans_get(&mtx, &self->pos_0x18);
                rotLocalMatX(self->rot_0x24.x, &mtx);
                if (self->type_0x02 == 13) {
                    setVector3(&scale, 1.0f, 1.0f, 1.0f);
                    setScaleMat34W(&mtx, &scale);
                }
                for (i = 0; i < work->count; i++) {
                    work->effects[i]->SetRootMtx(mtx);
                    effect_retire(work->effects[i], 0);
                    change_paramscale_eff(work->effects[i], work->scale_0x20 * get_em_scale(enemy));
                    eft_res_models_spawn((_EFT*)self, (void**)&work->effects[i], 1, 1, NULL);
                }
            }
        }
    }
}

/* State 1 of type 12: ends with the enemy; while the enemy is alive and not in sub-state 4 the effects are re-attached
 * to its joint (with the type's offset table), coloured grey by the enemy's glow level and handed on. */
extern "C" void fn_801063A8(_EFT013* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    _ENEMY_WORK* enemy;
    _EFT013_WORK_A* work;
    s32 i;
    _GXColor color;

    VEC3_ctor(&offset);
    enemy = self->source_0x30;
    work = (_EFT013_WORK_A*)self->work_0x38;
    MTX34_ctor(&mtx);
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (em_die_ck_c1(enemy) != 1 && enemy->state_0x1E2 != 4) {
        self->timer_0x0C++;
        for (i = 0; i < work->count; i++) {
            if (eft013_spawn_offsets[self->type_0x02] != NULL) {
                vec_to_mh_vec3(&offset, &eft013_spawn_offsets[self->type_0x02][i]);
                addVec3To(&offset, &work->pos_0x14);
            } else {
                copyVec3(&offset, &work->pos_0x14);
            }
            fn_801065FC(self, enemy, &offset, work->joint_0x10, (u8)i);
            effect_retire(work->effects[i], 0);
        }
        color.r = color.g = color.b = em018_glow_get(enemy);
        color.a = 255;
        change_color_eff(work->effects[0], &self->pos_0x18, color);
        self->area_0x44 = enemy->act_id;
        eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, NULL);
    }
}

/* 0x80106530 - state-2 handler: retire the emitter, tick the effect, then drive its colour/scale. */
extern "C" void fn_80106530(_EFT013* self) {
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);
    _EFT013_POOL* work = (_EFT013_POOL*)self->work_0x38;
    if (self->type_0x02 != 2) {
        self->state_0x05++;
        return;
    }
    switch (self->field_0x06) {
    case 0:
        self->field_0x06++;
        work->effects[0]->RetireEmitterAll();
    case 1:
        effect_retire(work->effects[0], 0);
        self->field_0x10--;
        if (self->field_0x10 < 0) {
            self->state_0x05++;
            return;
        }
        eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, 0);
        break;
    }
}

/* 0x801065F8 - destroy the effect object. */
extern "C" void fn_801065F8(_EFT013* self) {
    eft_res_slot_release(self);
}

/* 0x801065FC - place the indexed effect on an enemy joint: build the joint matrix, transform the
 * position into it, fold in the effect's own rotation and set the root matrix. */
extern "C" void fn_801065FC(_EFT013* self, _ENEMY_WORK* enemy, nw4r::math::VEC3* v, u32 joint, u32 index) {
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);
    _EFT013_POOL* work = (_EFT013_POOL*)self->work_0x38;
    get_joint_wmat_em(enemy, joint, &mtx);
    mulVecMat(v, &mtx);
    mtx34_trans_add(&mtx, v);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    work->effects[(u8)index]->SetRootMtx(mtx);
}

/* 0x80106694 (0x160): Spawns the enemy's one-effect record of `kind` at `pos` in the current area; a grounded enemy
 * (flags 6) turns kinds 0/1/4 into their landing variants and drops `pos` to its ground height. */
extern "C" void eft_spawn_type11(_ENEMY_WORK* self, nw4r::math::VEC3* pos, u32 kind, f32 scale)
{
    _EFT013* e;
    _EFT013_POOL* work;

    if (self->act_id != (u8)get_now_areano()) {
        return;
    }
    if ((self->joint_flags & 6) != 0) {
        switch (kind) {
        case 0:
            kind = 2;
            break;
        case 1:
            kind = 3;
            break;
        case 4:
            kind = 5;
            break;
        }
        pos->y = 5.0f + self->field_0x210;
    }
    e = (_EFT013*)eft_res_slot_get(16);
    if (e == NULL) {
        return;
    }
    work = (_EFT013_POOL*)e->work_0x38;
    work->count = 1;
    work->scale_0x08 = scale * get_em_scale(self);
    e->flag_0x01 = 1;
    e->field_0x03 = 11;
    e->type_0x02 = kind;
    copyVec3(&e->pos_0x18, pos);
    e->source_0x30 = self;
    e->area_0x44 = self->act_id;
    eft_state_flags_set((_EFT*)e, 0, 0);
    e->rot_0x24.x = 0;
    e->rot_0x24.y = self->pos_0x1BC.y;
    e->rot_0x24.z = 0;
    e->timer_0x0C = 0;
    if (event_demo_ck() == 1) {
        e->field_0x08 = 1;
    }
    e->release_0x40 = fn_801068E4;
    e->dispatch_0x34 = fn_80106920;
}

/* 0x801067F4 - spawn a type-11 enemy effect and install the fn_801068E4/fn_80106920 pair. */
extern "C" void fn_801067F4(u8 type, nw4r::math::VEC3* pos, f32 scale, u8 areano, u32 timer) {
    if ((u8)areano != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)eft_res_slot_get(16);
    if (e == NULL) {
        return;
    }
    _EFT013_POOL* work = (_EFT013_POOL*)e->work_0x38;
    work->count = 1;
    work->scale_0x08 = scale;
    e->flag_0x01 = 1;
    e->field_0x03 = 11;
    e->type_0x02 = type;
    copyVec3(&e->pos_0x18, pos);
    e->source_0x30 = NULL;
    e->area_0x44 = areano;
    e->rot_0x24.x = 0;
    e->rot_0x24.y = 0;
    e->rot_0x24.z = 0;
    e->timer_0x0C = timer;
    eft_state_flags_set((_EFT*)e, 0, 0);
    if (event_demo_ck() == 1) {
        e->field_0x08 = 1;
    }
    e->release_0x40 = fn_801068E4;
    e->dispatch_0x34 = fn_80106920;
}

/* 0x801068E4 - release the pool block (effects at +0x04). */
extern "C" void fn_801068E4(_EFT013* self) {
    _EFT013_POOL* pool = (_EFT013_POOL*)self->work_0x38;
    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* 0x80106920 - state_0x05 dispatcher for the fn_8010695C family. */
extern "C" void fn_80106920(_EFT013* self) {
    switch (self->state_0x05) {
    case 0:
        fn_8010695C(self);
        return;
    case 1:
        fn_80106ACC(self);
        return;
    case 2:
        fn_80106BA0(self);
        return;
    case 3:
        fn_80106BB0(self);
        return;
    default:
        return;
    }
}

/* The effect and parameter ids of the burst types (`fn_8010695C`) and of the shell types (`fn_80106F5C`), and the shell
 * types' colour per colour set (`fn_8010710C`: types 0..2 and 4..6). */
u16 eft013_burst_ids[8] = {0x00FB, 0x00FA, 0x01A3, 0x01A2, 0x033E, 0x033F, 0x00F9};
u16 eft013_burst_params[8] = {0x001F, 0x001F, 0x0028, 0x0028, 0x0032, 0x0032, 0x001F};
u16 eft013_shell_ids[8] = {0x0019, 0x002B, 0x002C, 0x0029, 0x00E4, 0x00E5, 0x00E6};
u16 eft013_shell_params[8] = {0x0005, 0x0005, 0x0005, 0x0006, 0x001B, 0x001B, 0x001B};
u8 eft013_shell_colors[20] = {0x08, 0x28, 0x08, 0xFF, 0x08, 0x2D, 0x0A, 0xFF, 0x0C, 0x23, 0x46, 0xFF,
                              0xE6, 0xD8, 0xE2, 0xFF, 0x28, 0x0A, 0x00, 0xFF};
u8 eft013_shell_colors_alt[20] = {0x08, 0x28, 0x08, 0xFF, 0x08, 0x28, 0x08, 0xFF, 0x0F, 0x28, 0x5A, 0xFF,
                                  0xE6, 0xD8, 0xE2, 0xFF, 0x20, 0x05, 0x00, 0xFF};

/* 0x8010695C - state-0 handler: create the effect from the per-type id tables, place it on the
 * effect's own rotation/position, recolour it by type and hand off to the state-1 handler. */
extern "C" void fn_8010695C(_EFT013* self) {
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);
    _EFT013_POOL* work = (_EFT013_POOL*)self->work_0x38;
    self->state_0x05++;
    work->effects[0] = res_eft_create(eft013_burst_ids[(u8)self->type_0x02], eft013_burst_params[(u8)self->type_0x02], 0);
    if (work->effects[0] == NULL) {
        fn_80106BB0(self);
        return;
    }
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    mtx.m[0][3] = self->pos_0x18.x;
    mtx.m[1][3] = self->pos_0x18.y;
    mtx.m[2][3] = self->pos_0x18.z;
    work->effects[0]->SetRootMtx(mtx);
    change_paramscale_eff(work->effects[0], work->scale_0x08);
    u32 color;
    switch (self->type_0x02) {
    case 0:
    case 1:
    case 4:
    case 6:
        color = get_stg_eft_col(self->area_0x44, 0);
        break;
    case 2:
    case 3:
    case 5:
        color = get_stg_eft_col(self->area_0x44, 1);
        fn_800DC60C(&self->pos_0x18, 2);
        break;
    default:
        break;
    }
    work->color_0x0C.r = (color & 0xFF000000) >> 24;
    work->color_0x0C.g = (color & 0x00FF0000) >> 16;
    work->color_0x0C.b = (color & 0x0000FF00) >> 8;
    work->color_0x0C.a = 255;
    fn_80106ACC(self);
}

/* 0x80106ACC - state-1 handler: tick the timer or the effect, recolour it from the work block and
 * drive its colour/scale. */
extern "C" void fn_80106ACC(_EFT013* self) {
    _EFT013_POOL* work = (_EFT013_POOL*)self->work_0x38;
    if (self->field_0x08 == 1) {
        if (event_demo_ck() == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    if (self->timer_0x0C > 0) {
        self->timer_0x0C--;
        return;
    }
    if (effect_move(work->effects[0]) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    _GXColor color = work->color_0x0C;
    change_color_eff(work->effects[0], &self->pos_0x18, color);
    eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, 0);
}

/* 0x80106BA0 - the trivial state advance. */
extern "C" void fn_80106BA0(_EFT013* self) {
    self->state_0x05++;
}

/* 0x80106BB0 - destroy the effect object. */
extern "C" void fn_80106BB0(_EFT013* self) {
    eft_res_slot_release(self);
}

/* Spawns a shell-coloured one-effect record of `type` at `pos` for an actor in the current area (the first types are
 * refused for a bare kind-3 actor, and all but type 3 while option 21 is set); the scale and colour set follow the
 * actor's shell. */
extern "C" void eft013_shell_spawn(nw4r::math::VEC3* pos, u32 type, _CP_VECTOR* rot, f32 scale, _EFT013_SHELL_SRC* src)
{
    _EFT013* e;
    _EFT013_POOL2* work;

    if (get_option_21() != 0 && type != 3) {
        return;
    }
    if (src->area_0x0F != (u8)get_now_areano()) {
        return;
    }
    if ((type <= 2 || type - 4 <= 2) && src->kind_0x30 == 3 && src->field_0x06 == 0) {
        return;
    }
    e = (_EFT013*)eft_res_slot_get(16);
    if (e == NULL) {
        return;
    }
    e->source_0x30 = NULL;
    work = (_EFT013_POOL2*)e->work_0x38;
    work->count = 1;
    work->scale_0x04 = scale;
    e->field_0x03 = 12;
    e->type_0x02 = type;
    copyVec3(&e->pos_0x18, pos);
    e->rot_0x24.x = rot->x;
    e->rot_0x24.y = rot->y;
    e->rot_0x24.z = 0;
    eft_state_flags_set((_EFT*)e, 0, 0);
    e->area_0x44 = src->area_0x0F;
    if ((s32)src->kind_0x30 == 1) {
        e->field_0x08 = 1;
        switch (src->shell_0x2C->type_0x03) {
        case 26:
        case 30:
        case 31:
            work->scale_0x08 = 0.5f;
            e->field_0x07 = 4;
            break;
        case 36:
        case 37:
        case 38:
        case 39:
            e->field_0x07 = 0;
            work->scale_0x08 = 0.5f;
            break;
        case 33:
            e->field_0x07 = 1;
            work->scale_0x08 = 0.5f;
            break;
        case 32:
            if ((u32)(src->shell_0x2C->level_0x0A - 3) <= 1) {
                e->field_0x07 = 2;
            } else {
                e->field_0x07 = 4;
            }
            work->scale_0x08 = 1.0f;
            break;
        case 35:
            work->scale_0x08 = 1.0f;
            e->field_0x07 = 3;
            break;
        default:
            work->scale_0x08 = 1.0f;
            e->field_0x07 = 4;
            break;
        }
    } else {
        work->scale_0x08 = 1.0f;
        e->field_0x07 = 4;
        e->field_0x08 = 0;
    }
    e->release_0x40 = fn_80106EE4;
    e->dispatch_0x34 = fn_80106F20;
}

/* 0x80106DC8 - spawn a type-3 effect on the caller's position/rotation and install the
 * fn_80106EE4/fn_80106F20 pair. */
extern "C" void fn_80106DC8(nw4r::math::VEC3* pos, u32* rot, f32 scale, _EFT013_SRC* src) {
    if (src->area_0x0F != get_now_areano()) {
        return;
    }
    _EFT013* e = (_EFT013*)eft_res_slot_get(16);
    if (e == NULL) {
        return;
    }
    if (src->field_0x30 == 0) {
        e->source_0x30 = (_ENEMY_WORK*)src->field_0x2C;
        e->field_0x06 = 1;
    } else {
        e->source_0x30 = NULL;
        e->field_0x06 = 0;
    }
    _EFT013_POOL2* work = (_EFT013_POOL2*)e->work_0x38;
    work->count = 1;
    work->scale_0x04 = scale;
    e->field_0x03 = 12;
    e->type_0x02 = 3;
    copyVec3(&e->pos_0x18, pos);
    e->rot_0x24.x = rot[0];
    e->rot_0x24.y = rot[1];
    e->rot_0x24.z = 0;
    e->area_0x44 = src->area_0x0F;
    eft_state_flags_set((_EFT*)e, 0, 0);
    e->release_0x40 = fn_80106EE4;
    e->dispatch_0x34 = fn_80106F20;
}

/* 0x80106EE4 - release the pool block (effects at +0x0C). */
extern "C" void fn_80106EE4(_EFT013* self) {
    _EFT013_POOL2* pool = (_EFT013_POOL2*)self->work_0x38;
    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* 0x80106F20 - state_0x05 dispatcher for the fn_80106F5C family. */
extern "C" void fn_80106F20(_EFT013* self) {
    switch (self->state_0x05) {
    case 0:
        fn_80106F5C(self);
        return;
    case 1:
        fn_8010710C(self);
        return;
    case 2:
        fn_8010723C(self);
        return;
    case 3:
        fn_8010724C(self);
        return;
    default:
        return;
    }
}

/* 0x80106F5C (0x1B0): Creates and scales the per-type effect, places every pooled effect, recolours by type and
 * hands off to the state-1 handler. */
extern "C" void fn_80106F5C(_EFT013* self) {
    s32 i;
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    _EFT013_POOL2* work = (_EFT013_POOL2*)self->work_0x38;
    self->state_0x05++;
    work->effects[0] = res_eft_create(eft013_shell_ids[(u8)self->type_0x02], eft013_shell_params[(u8)self->type_0x02], 0);
    if (work->effects[0] == NULL) {
        fn_8010724C(self);
        return;
    }
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    if (self->type_0x02 <= 2) {
        work->scale_0x04 *= work->scale_0x08;
    }
    setVector3(&v, work->scale_0x04, work->scale_0x04, work->scale_0x04);
    fn_80050708(&mtx, &v);
    for (i = 0; i < work->count; i++) {
        work->effects[i]->SetRootMtx(mtx);
        fn_800F975C(work->effects[i], &self->pos_0x18);
    }
    self->flag_0x01 = 1;
    if (self->field_0x08 == 1) {
        switch (self->type_0x02) {
        case 0:
            fn_800DC6D8(&self->pos_0x18, 2);
            break;
        case 1:
            fn_800DC6D8(&self->pos_0x18, 1);
            break;
        case 2:
            fn_800DC6D8(&self->pos_0x18, 0);
            break;
        default:
            break;
        }
    }
    if (self->field_0x06 == 1 && (s32)self->type_0x02 == 3) {
        fn_800DB974(self->source_0x30, &self->pos_0x18);
    }
    fn_8010710C(self);
}

/* 0x8010710C - state-1 handler for the fn_80106F5C family: recolour the effect from the per-index
 * colour tables (types 0-2 vs 4-6) and drive its colour/scale. */
extern "C" void fn_8010710C(_EFT013* self) {
    _GXColor color;
    _EFT013_POOL2* work = (_EFT013_POOL2*)self->work_0x38;
    if (effect_move(work->effects[0]) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2: {
        u8 idx = self->field_0x07;
        color.r = eft013_shell_colors[idx * 4 + 0];
        color.g = eft013_shell_colors[idx * 4 + 1];
        color.b = eft013_shell_colors[idx * 4 + 2];
        color.a = eft013_shell_colors[idx * 4 + 3];
        change_color_eff(work->effects[0], &self->pos_0x18, color);
        break;
    }
    case 4:
    case 5:
    case 6: {
        u8 idx = self->field_0x07;
        color.r = eft013_shell_colors_alt[idx * 4 + 0];
        color.g = eft013_shell_colors_alt[idx * 4 + 1];
        color.b = eft013_shell_colors_alt[idx * 4 + 2];
        color.a = eft013_shell_colors_alt[idx * 4 + 3];
        change_color_eff(work->effects[0], &self->pos_0x18, color);
        break;
    }
    default:
        break;
    }
    eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, 0);
}

/* 0x8010723C - the trivial state advance. */
extern "C" void fn_8010723C(_EFT013* self) {
    self->state_0x05++;
}

/* 0x8010724C - destroy the effect object. */
extern "C" void fn_8010724C(_EFT013* self) {
    eft_res_slot_release(self);
}

