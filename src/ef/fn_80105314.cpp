/* ef/fn_80105314.cpp - the effect band at 0x80105314
 *
 * `.text` 0x80105314..0x80107250, 28 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit; the functions of the neighbouring units were cut out of this file.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 */

/* Retired header of `ef/fn_80105314.cpp` (kept for its notes and residuals): */
/* ef/fn_80105314.cpp - the enemy/player effect-setter batch, .text 0x80105314..0x8010BDE4 (93
 * functions, 27344 bytes).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt; only `eft013_set`,
 * `eft013_set_dmeft_pl`, `eft014_set_daihouden` and `eft015_set` carry a real name).
 *
 * Registration evidence (brief §2).  No `__FILE__` string: `strings` over
 * `orig/RMHE08/sys/main.dol` finds no `.cpp`/`.c` source name and no `eft0NN` token at all, so class 1
 * is empty.  Class 2 (`tools/symbols/dumpmap.py lookup`) names only four symbols, and they span three
 * different families (`eft013_set` 0x80107250, `eft013_set_dmeft_pl` 0x80107784,
 * `eft014_set_daihouden` 0x8010A988, `eft015_set` 0x8010BBE0) - a single file name is not supported by
 * them, so the range is *not* one of the `eft0NN.cpp` TUs.  Class 3: the code is the same `_EFT`
 * effect-object state machine the bracketing `ef/fn_80104BD0.c` (100 %) and `ef/fn_8010D1A8.c` units
 * are, and those two neighbours both use the map's `fn_<address>` stem as their file name.  Class 4
 * therefore decides it: the file keeps the map's `fn_80105314` stem, and the module is `ef` (both
 * bracketing registered units are `ef/`).  The seam is unproven - the discovery capped this at
 * --max-bytes and the run plainly holds several original families (see the inventory below).
 *
 * Language: C++.  The range's own definition `eft013_set__FP4_PLWUc` is mangled (MWCC `__F` argument
 * list), which is langcheck's conclusive signal; a C++ definition would mangle unless it is
 * `extern "C"`, so every definition whose map name is plain (`fn_80105314`, ...) is `extern "C"` and
 * objdiff pairs it by name.  The mangled *callees* are declared with the signature their map name
 * encodes (rule 9), not with the mangled spelling.
 *
 * Inventory in address order (family clusters):
 *   0x80105314..0x8010724C  the `_EFT` enemy/emitter base cluster: per-frame handlers
 *                           (`fn_80105314`, `eft_spawn_type10`, ...), state dispatchers
 *                           (`fn_801059AC`, `fn_80106920`, `fn_80106F20`), pool releases
 *                           (`fn_80105970`, `fn_801068E4`, `fn_80106EE4`), type dispatch tables
 *                           (`fn_80105564`, `fn_80105EFC`) and the setters (`fn_801057A4`,
 *                           `fn_80105888`, ...).
 *   0x80107250..0x80107DDC  the `eft013` cluster (`eft013_set`, `eft013_set_dmeft_pl`, helpers).
 *   0x80107E5C..0x8010A8CC  the large player-weapon handlers and their setters.
 *   0x8010A988..0x8010B154  the `eft014` cluster (`eft014_set_daihouden`).
 *   0x8010B198..0x8010BDA8  the `eft015` cluster (`eft015_set`).
 *
 * Flags: the whole unit runs with the peephole pass off (`#pragma peephole off`, playbook 39) - the
 * target object carries no fused record forms (tools/flags/infer.py on
 * `build/RMHE08/obj/auto_fn_80105314_text.o`: peephole off [med], 0 record forms).
 *
 * Data: the unit owns no pool section; its `.data` jump tables (`jumptable_8059E9AC`, ...), `.sdata`
 * type tables (`lbl_80791808`) and `.sdata2` floats are the shared pool, `extern`-declared here and
 * never defined (playbook 29).
 *
 * Residuals are recorded per function below as the reconstruction proceeds.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "ef/eft004.h"
#include "ef/fn_80105314.h"
#include "ef/eft_res.h"
#include "ef/fn_8010BDE4.h"
#include "gx.h"
#include "pl.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "ef/fn_80105314_types.h"

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
    /* +0x04 */ u8 unused_0x04[0x10 - 0x04];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ nw4r::math::VEC3 pos_0x14;
    /* +0x20 */ f32 scale_0x20;
};
/* size: 0x24 - lower bound, an approximation. */

/* ---------------------------------------------------------------------------------------------------
 * externs - mangled callees are declared through their real signature (rule 9)
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_80050708(void* mtx, nw4r::math::VEC3* v);

extern "C" u8 lbl_8059E688[];
extern "C" u16 lbl_8059EAB0[];
extern "C" u16 lbl_8059EAC0[];
extern "C" u8 lbl_8059EAF0[];
extern "C" u8 lbl_8059EB04[];
extern "C" u16 lbl_8059EAD0[];
extern "C" u16 lbl_8059EAE0[];

extern "C" f32 lbl_80796788;

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
/* `se_req_pos_ps` comes from the owner's header `sound/fn_800D7F54.h` (rule 2); this unit's local
 * `void` copy collided with the owner's `SeSlot*` once the header declared it. */
u8 get_now_areano();
u32 em_sleep_ck(_ENEMY_WORK* enemy, u8 kind);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* mtx);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
int event_demo_ck();

extern "C" _EFT013* fn_80105888(_ENEMY_WORK* self, u32 type, u32 a, nw4r::math::VEC3* b, f32 scale);

/* ---------------------------------------------------------------------------------------------------
 * state dispatchers and pool releases
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_801059E8(_EFT013* self);
extern "C" void fn_80105EFC(_EFT013* self);
extern "C" void fn_80105F30(_EFT013* self);
extern "C" void fn_80106180(_EFT013* self);
extern "C" void fn_801063A8(_EFT013* self);
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

#pragma peephole off

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

/* 0x80105EFC - type dispatch table (types 0..32).  Every value 0..32 is a case: the compiler emitted a
 * 33-entry jump table with no default body (out-of-range returns), so the non-case values fall to the
 * fn_80105F30 body through the shared run (the table is `jumptable_8059E9AC`, read from the DOL). */
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

/* 0x801065F8 - destroy the effect object. */
extern "C" void fn_801065F8(_EFT013* self) {
    eft_res_slot_release(self);
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

/* 0x80106BA0 - the trivial state advance. */
extern "C" void fn_80106BA0(_EFT013* self) {
    self->state_0x05++;
}

/* 0x80106BB0 - destroy the effect object. */
extern "C" void fn_80106BB0(_EFT013* self) {
    eft_res_slot_release(self);
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

/* 0x8010723C - the trivial state advance. */
extern "C" void fn_8010723C(_EFT013* self) {
    self->state_0x05++;
}

/* 0x8010724C - destroy the effect object. */
extern "C" void fn_8010724C(_EFT013* self) {
    eft_res_slot_release(self);
}

#pragma peephole on

/* 0x80106F5C - state-0 handler for the fn_80106F5C family: create the per-type effect, scale it (the
 * type<=2 group multiplies the spawn scale into it), place every pooled effect on the matrix and
 * position, then recolour by type and hand off to the state-1 handler. */
extern "C" void fn_80106F5C(_EFT013* self) {
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    _EFT013_POOL2* work = (_EFT013_POOL2*)self->work_0x38;
    self->state_0x05++;
    work->effects[0] = res_eft_create(lbl_8059EAD0[(u8)self->type_0x02], lbl_8059EAE0[(u8)self->type_0x02], 0);
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
    for (s32 i = 0; i < work->count; i++) {
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
    if (self->field_0x06 == 1 && self->type_0x02 == 3) {
        fn_800DB974(self->source_0x30, &self->pos_0x18);
    }
    fn_8010710C(self);
}

/* 0x8010710C - state-1 handler for the fn_80106F5C family: recolour the effect from the per-index
 * colour tables (types 0-2 vs 4-6) and drive its colour/scale. */
extern "C" void fn_8010710C(_EFT013* self) {
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
        _GXColor color;
        color.r = lbl_8059EAF0[idx * 4 + 0];
        color.g = lbl_8059EAF0[idx * 4 + 1];
        color.b = lbl_8059EAF0[idx * 4 + 2];
        color.a = lbl_8059EAF0[idx * 4 + 3];
        change_color_eff(work->effects[0], &self->pos_0x18, color);
        break;
    }
    case 4:
    case 5:
    case 6: {
        u8 idx = self->field_0x07;
        _GXColor color;
        color.r = lbl_8059EB04[idx * 4 + 0];
        color.g = lbl_8059EB04[idx * 4 + 1];
        color.b = lbl_8059EB04[idx * 4 + 2];
        color.a = lbl_8059EB04[idx * 4 + 3];
        change_color_eff(work->effects[0], &self->pos_0x18, color);
        break;
    }
    default:
        break;
    }
    eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, 0);
}

/* 0x8010695C - state-0 handler: create the effect from the per-type id tables, place it on the
 * effect's own rotation/position, recolour it by type and hand off to the state-1 handler. */
extern "C" void fn_8010695C(_EFT013* self) {
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);
    _EFT013_POOL* work = (_EFT013_POOL*)self->work_0x38;
    self->state_0x05++;
    work->effects[0] = res_eft_create(lbl_8059EAB0[(u8)self->type_0x02], lbl_8059EAC0[(u8)self->type_0x02], 0);
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

/* 0x80106DC8 - spawn a type-3 effect on the caller's position/rotation and install the
 * fn_80106EE4/fn_80106F20 pair. */
extern "C" void fn_80106DC8(nw4r::math::VEC3* pos, u32* rot, _EFT013_SRC* src, f32 scale) {
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
        fn_800F996C(work->effects[0], 0);
        break;
    default:
        return;
    }
    self->field_0x10--;
    if (self->field_0x10 < 0) {
        self->state_0x05++;
        return;
    }
    eft_res_models_spawn((_EFT*)self, (void**)work->effects, 1, work->count, 0);
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

/* 0x801067F4 - spawn a type-11 enemy effect and install the fn_801068E4/fn_80106920 pair. */
extern "C" void fn_801067F4(u32 type, nw4r::math::VEC3* pos, f32 scale, u32 areano, u32 timer) {
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
    e->type_0x02 = (u8)type;
    copyVec3(&e->pos_0x18, pos);
    e->source_0x30 = NULL;
    e->area_0x44 = (u8)areano;
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

/* 0x8010562C - spawn an enemy effect: pool the object, seed the work block from the type table, gate on
 * the area and the enemy's sleep state, then copy the caller's position into either the effect's own
 * `pos_0x18` or the work block's `pos_0x14` (the type switch). */
extern "C" void eft_spawn_type10(_ENEMY_WORK* self, u32 type, u32 a, nw4r::math::VEC3* b, f32 scale) {
    _EFT013* e = (_EFT013*)eft_res_slot_get(0x24);
    if (e == NULL) {
        return;
    }
    _EFT013_WORK_A* work = (_EFT013_WORK_A*)e->work_0x38;
    work->count = lbl_8059E688[(u8)type];
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
    work->field_0x10 = a;
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
    _EFT013* e = fn_80105888(self, (u8)type, a, b, lbl_80796788);
    if (e != NULL) {
        e->rot_0x24.x = 0;
        eft_state_flags_set((_EFT*)e, 1, 0);
    }
}

/* 0x80105888 - the allocator every `fn_801057xx` setter funnels through: pool a 0x24-slot object,
 * seed the work block from the type table, copy the enemy position and install the release/dispatch
 * pair. */
extern "C" _EFT013* fn_80105888(_ENEMY_WORK* self, u32 type, u32 a, nw4r::math::VEC3* b, f32 scale) {
    _EFT013* e = (_EFT013*)eft_res_slot_get(0x24);
    if (e == NULL) {
        return NULL;
    }
    _EFT013_WORK_A* work = (_EFT013_WORK_A*)e->work_0x38;
    work->count = lbl_8059E688[(u8)type];
    work->field_0x10 = a;
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
