/* ef/eft022_fx.cpp - the eft021 and eft022 effect families and the eft023/eft024 job machines.  Each creator takes the
 *   0x48-byte `_EFT` record with a work block from `eft_res_slot_get`, stamps `+0x03` and installs its hooks:
 *   21 `fn_80115460` (0x25C `_EFT_WORK_B`, release `fn_80115554`, dispatch `fn_80115664`), 22 `eft022_set` (0x8
 *   `_EFT_WORK_C`, `fn_801164F0`, `fn_8011652C`), 23 `fn_80116770` (0x4C `_EFT_JOB_WORK`, `fn_8011688C`,
 *   `fn_8011689C`), 24 `fn_80116DA4` (0x84 `_EFT_JOB2_WORK`, `fn_80116FCC`, `fn_80116FDC`).
 * RANGE. .text 0x801153D0-0x8011722C (37 functions); extab 0x8000C3F4-0x8000C494, extabindex 0x800264D8-0x800265C8,
 *   .data 0x805A0208-0x805A0460, .sdata 0x80791930-0x80791938, .sdata2 0x80796A30-0x80796A80.  Family 20 below
 *   0x801153D0 is `ef/fn_80114E34.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (playbook 39) and `#pragma unroll off` around
 *   `fn_80117120`.
 * NAMES. `eft022_set` is the runtime dump's own name; the map has only `fn_` stems for the rest, so plain definitions
 *   are `extern "C"`.
 * RESIDUALS. 17 partial rows, including:
 *  - `fn_80116080`: the pose-blend loop's float scheduling;
 *  - `fn_80116FDC`: block layout (the case bodies sit after the switch exit in retail);
 *  - `fn_80116B00`: the unsigned int-to-float conversion (`xoris`).
 *   The other 14 partial rows have no recorded cause (`symdiff.py -u ef/eft022_fx --all`).
 *   flipcheck: `.data`/`.sdata` claimed, not emitted; `.text` (0x1E2C of 0x1E5C) and `.sdata2` (0x10 of 0x50) short of
 *   the claim; `.text`, `.sdata2`, extab and extabindex differing.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_8050131C`,
 *     `MTX34Zero__Q24nw4r4mathFPQ34nw4r4math5MTX34`, `fn_80115FB0`, `_savegpr_15`, `_savegpr_16`, `_restgpr_15`,
 *     `_restgpr_16`, `_savegpr_21`, `_savegpr_22`, `_restgpr_21`, `_restgpr_22`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_8050131C`.
 * SHAPES. `fn_80116EEC` nests `for (j = 0; j < 4; j++) for (k = 0; k < 8; k++)` (retail's `mtctr`/`bdnz` and the
 *   `mr r3,r4` early returns; a flat `do`/`while` emits a decrement and compare).
 *   `fn_80117120` reaps one slot per iteration.
 *   `fn_80116FDC` uses the `case 0: break; case 1: break;` plus after-switch form (docs/ef.md, "The case-0 nested
 *   dispatch"); bodies inside the cases stay inline.
 *   `fn_80116B00` loads its seven float constants into named locals before the loop (inline globals reload per use).
 */

#include "ef/eft_res_model_get.h" /* eft_res_model_get (rule 2: the owner's header) */
#include "ef/eft_res_slot_get.h" /* eft_res_slot_get (rule 2: the owner's header) */
#include "ef/eft_res_slot_release.h" /* eft_res_slot_release (rule 2: the owner's header) */
#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "ef/eft_res_spawn_gate_ck.h" /* eft_res_spawn_gate_ck (rule 2: the owner's header) */
#include "ef/eft_res_models_spawn.h" /* eft_res_models_spawn (rule 2: the owner's header) */
#include "ef/mtx34_trans_get.h" /* mtx34_trans_get (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "enemy/fn_8012BDF4.h"
#include "sys_mem.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "ef/fn_8011722C.h"
#include "g3d/g3d_scnmdlsmpl.h" /* fn_80080B10/fn_800810DC (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_8006FDCC (rule 2) */
#include "unsplit/sound.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_80114E34_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_res_model_get_c1 ((void* (*)(void))eft_res_model_get)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800F8788_c1 ((_EFT* (*)(u32))eft_res_slot_get)
#define fn_800F9DF4_c1 ((void (*)(_EFT*, u8, u8))eft_state_flags_set)
#define fn_800F92F4_c1 ((u32 (*)(_EFT*, u32))eft_res_spawn_gate_ck)
#define fn_800F93D8_c1 ((void (*)(void*, void*, u32, s32, u32))eft_res_models_spawn)
#define mtx34_trans_get_c1 ((void (*)(nw4r::math::MTX34*, nw4r::math::VEC3*, f32))mtx34_trans_get)

/* The engine's own 3-float vector.  It is NOT `nw4r::math::VEC3`: `vec_to_mh_vec3` exists to convert
 * between the two (`nw4r::math::VEC3* dst, Vec* src`), so they are distinct types that happen to share
 * a layout.  `ef.h` carries the canonical copy; this unit views its records locally, so it repeats the
 * type here (docs/plan.md 6.5 rule 1 debt, tracked in the campaign note). size: 0x0C */
typedef struct Vec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} Vec; /* size: 0x0C */

/* The channel selector `setMatColor` takes; the call sites pass `GX_COLOR0A0` (4). */
enum _GXChannelID {
    GX_COLOR0,
    GX_COLOR1,
    GX_ALPHA0,
    GX_ALPHA1,
    GX_COLOR0A0,
    GX_COLOR1A1,
    GX_COLORZERO,
    GX_ALPHA0A0,
    GX_ALPHA1A1,
    GX_ALPHAZERO
};

/* A model object the eft021 record owns: its vtable, its owner and its slot index. size: 0x0C */
struct _EFT_MODEL_OBJ {
    /* +0x00 */ void** vtable;
    /* +0x04 */ _EFT* owner_0x04;
    /* +0x08 */ s32 index_0x08;
};

/* The eft021 work block. size: 0x25C */
struct _EFT_WORK_B {
    /* +0x000 */ s32 count;
    /* +0x004 */ MHchar* models[2];
    /* +0x00C */ nw4r::math::MTX34 mtx[2][6];
    /* +0x24C */ void* objects[2];
    /* +0x254 */ s32 joint_num;
    /* +0x258 */ s16 field_0x258;
    /* +0x25A */ s16 field_0x25A;
};

/* The eft022 work block. size: 0x08 */
struct _EFT_WORK_C {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
};

/* size: 0x148 (lower bound) */

/* One entry of the shared spawn-offset tables the eft023 models are placed from. size: 0x1C */
struct _EFT_MAP_TABLE {
    /* +0x00 */ u8 unused_0x00[0x04];
    /* +0x04 */ nw4r::math::VEC3 pos_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 field_0x11;
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ f32 field_0x14;
    /* +0x18 */ u8 field_0x18;
    /* +0x19 */ u8 unused_0x19;
    /* +0x1A */ s16 field_0x1A;
    /* +0x1C */ u8 unused_0x1C[0x1C - 0x1C];
};

/* The eft023 work block. size: 0x4C */
struct _EFT_JOB_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[5];
    /* +0x18 */ s16 field_0x18[5];
    /* +0x22 */ s16 field_0x22[5];
    /* +0x2C */ s16 field_0x2C[5];
    /* +0x38 */ _EFT_MAP_TABLE* tables[5];
};

/* The eft024 work block: 32 character slots and the live counters. size: 0x84 */
struct _EFT_JOB2_WORK {
    /* +0x00 */ _EFT* slots[32];
    /* +0x80 */ s16 total;
    /* +0x82 */ s16 per_type[1];
};

extern "C" void* fn_801153D0(void*, s32, u8);
extern "C" void* fn_80115460(u8);
extern "C" void fn_80115554(_EFT*);
extern "C" s32 fn_80115608(void*, s16);
extern "C" void fn_80115664(_EFT*);
extern "C" void fn_801156A0(_EFT*);
extern "C" void fn_80115A44(void**);
extern "C" void fn_80115A80(_EFT*);
extern "C" void fn_80115FA0(_EFT*);
extern "C" void fn_80115FB0(_EFT*);
extern "C" void fn_80115FB4(_EFT_MODEL_OBJ*);
extern "C" void fn_80116080(_EFT_MODEL_OBJ*, nw4r::math::MTX34*, s32);
extern "C" void fn_80116430(void*, nw4r::math::VEC3*, u8);
extern "C" void fn_801164F0(_EFT*);
extern "C" void fn_8011652C(_EFT*);
extern "C" void fn_80116568(_EFT*);
extern "C" void fn_8011661C(_EFT*);
extern "C" void fn_8011675C(_EFT*);
extern "C" void fn_8011676C(_EFT*);
extern "C" void fn_80116770(void);
extern "C" void fn_8011688C(_EFT*);
extern "C" void fn_8011689C(_EFT*);
extern "C" void fn_801168D8(_EFT*);
extern "C" void fn_80116B00(_EFT*);
extern "C" void fn_80116D90(_EFT*);
extern "C" void fn_80116DA0(_EFT*);
extern "C" void fn_80116DA4(void);
extern "C" void* fn_80116E14(s16);
extern "C" void fn_80116E1C(_EFT*, s16);
extern "C" s16 fn_80116EEC(_EFT*);
extern "C" void fn_80116FCC(_EFT*);
extern "C" void fn_80116FDC(_EFT*);
extern "C" void fn_80117074(_EFT*);
extern "C" void fn_80117084(_EFT*);
extern "C" void fn_80117088(_EFT*);
extern "C" void fn_80117120(_EFT*);

/* The library entry points, declared by their map spelling (a mangled callee is not evidence of a
 * C++ unit; only a mangled definition is). */
extern "C" void fn_800F8A44(void** effects, s32 count);


extern "C" void mtx34_copy(void* dst, void* src);

extern "C" void fn_80059374(s32 a);
extern "C" void fn_80059420(void);
extern "C" void fn_8005D1AC(void* out, s32 a);
extern "C" void fn_8005D0CC(void* ctx, void* val);
extern "C" s32 fn_8005D050(void* ctx);
extern "C" u32 fn_8005AAEC(void* ctx);
extern "C" s32 fn_80097DE4(void* a, void* b);
extern "C" s32 fn_80097F18(s32 a, u32 b);
extern "C" void fn_8050131C(void* a);
extern "C" f32 fn_80463EE0(s16 a, f32 b);
extern "C" void fn_80051894(void* a, void* b, void* c, s32 d, f32 e, f32 f);

extern "C" u32 get_joint_num__6MHcharFv(void* chr);
extern "C" void setVisibility__6MHcharFUlb(void* chr, u32 joint, u32 visible);
extern "C" void setMatColor__6MHcharFUl12_GXChannelID8_GXColorb(void* chr, u32 idx,
                                                                _GXChannelID channel,
                                                                _GXColor color, u32 keep);
extern "C" void move__6MHcharFUs(void* chr, u16 a);
extern "C" void move2__6MHcharFPQ34nw4r4math5MTX34Us(void* chr, nw4r::math::MTX34* m, u16 a);

extern "C" nw4r::math::VEC3 lbl_805A0208[];
extern "C" f32 lbl_805A0244[];
extern "C" _GXColor lbl_805A0258[];
extern "C" _GXColor lbl_805A026C[];
extern "C" nw4r::math::VEC3* lbl_805A0300[];
extern "C" nw4r::math::VEC3* lbl_805A0330[];
extern "C" u8 lbl_805A0360[];
extern "C" _EFT_MAP_TABLE lbl_805A0380[];
extern "C" _EFT_MAP_TABLE lbl_805A039C[];
extern "C" _EFT_MAP_TABLE lbl_805A0428[];

extern "C" char lbl_80791930[8];

extern "C" f32 lbl_80796A30;
extern "C" f32 lbl_80796A34;
extern "C" f32 lbl_80796A38;
extern "C" f32 lbl_80796A3C;
extern "C" f32 lbl_80796A40;
extern "C" f32 lbl_80796A44;
extern "C" f32 lbl_80796A48;

extern "C" f64 lbl_80796A58;
extern "C" f32 lbl_80796A60;
extern "C" f32 lbl_80796A64;
extern "C" f32 lbl_80796A68;
extern "C" f32 lbl_80796A6C;
extern "C" f32 lbl_80796A70;
extern "C" f32 lbl_80796A74;
extern "C" f64 lbl_80796A78;

#pragma peephole off

/* Creates the actor's eft021 record and seeds its work block. */
extern "C" void* fn_801153D0(void* source, s32 timer, u8 type)
{
    _EFT* effect;
    _EFT_WORK_B* work;

    effect = (_EFT*)fn_80115460(type);
    if (effect == 0) {
        return 0;
    }
    work = (_EFT_WORK_B*)effect->work_0x38;
    work->field_0x258 = Get_motion_no((_PLW*)source);
    work->field_0x25A = -1;
    effect->source_0x30 = source;
    effect->area_0x44 = ((_PLW*)source)->area_0x16;
    effect->timer_0x0C = timer;
    return effect;
}

/* Creates the eft021 record: one or two models, each with its own effect object. */
extern "C" void* fn_80115460(u8 type)
{
    _EFT* effect;
    _EFT_WORK_B* work;
    s32 i;

    effect = fn_800F8788_c1(0x25C);
    if (effect == 0) {
        return 0;
    }
    effect->type_0x02 = type;
    effect->release_0x40 = fn_80115554;
    work = (_EFT_WORK_B*)effect->work_0x38;
    if (type != 2) {
        work->count = 1;
    } else {
        work->count = 2;
    }
    for (i = 0; i < work->count; i++) {
        work->objects[i] = 0;
        work->models[i] = (MHchar*)eft_res_model_get_c1();
        if (work->models[i] == 0) {
            eft_res_slot_release(effect);
            return 0;
        }
    }
    effect->field_0x03 = 21;
    fn_800F9DF4_c1(effect, 1, 0);
    effect->dispatch_0x34 = fn_80115664;
    return effect;
}

/* Releases the eft021 record's models and effect objects. */
extern "C" void fn_80115554(_EFT* self)
{
    _EFT_WORK_B* work;
    s32 i;
    s32 j;
    _EFT_MODEL_OBJ* object;

    work = (_EFT_WORK_B*)self->work_0x38;
    fn_800F8A44((void**)work->models, work->count);
    for (i = 0; i < work->count; i++) {
        for (j = 0; j < 6; j++) {
            fn_8050131C(&work->mtx[i][j]);
        }
        object = (_EFT_MODEL_OBJ*)work->objects[i];
        if (object != 0) {
            (*(void (*)(void*, s32))object->vtable[6])(object, 1);
        }
    }
    work->count = 0;
    work->field_0x258 = 0;
    work->field_0x25A = 0;
}

/* Frees an eft021 work block once the record is gone. */
extern "C" s32 fn_80115608(void* p, s16 flag)
{
    if (p != 0) {
        fn_800810DC(p, 0);
        if (flag > 0) {
            operator delete(p);
        }
    }
    return (s32)p;
}

/* Runs the state handler the eft021 record's `state_0x05` selects. */
extern "C" void fn_80115664(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_801156A0(self);
    case 1:
        return fn_80115A80(self);
    case 2:
        return fn_80115FA0(self);
    case 3:
        return fn_80115FB0(self);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * eft022 (fn_801164F0 release, fn_8011652C dispatch)
 * ------------------------------------------------------------------------------------------------- */

/* Bumps the record's `state_0x05` state index. */
extern "C" void fn_80115FA0(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the eft021 record: the family-21 state-3 step. */
extern "C" void fn_80115FB0(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Releases the eft021 model's texture handles. */
extern "C" void fn_80115FB4(_EFT_MODEL_OBJ* obj)
{
    _EFT* effect = obj->owner_0x04;
    _EFT_WORK_B* work = (_EFT_WORK_B*)effect->work_0x38;
    s32 h;
    s32 id;
    s32 i;

    fn_8005D1AC(&h, 0);
    for (i = 0; i < work->count; i++) {
        s32 handle = work->models[i]->field_0x118;
        fn_80080B10((void*)handle, 4);   /* the declaration takes void*, as its C sibling unit casts */
        id = fn_80097DE4(&work->models[i]->field_0x114, lbl_80791930);
        fn_8005D0CC(&h, &id);
        if (fn_8005AAEC(&h) == 1U) {
            fn_800E3264((void*)handle, fn_8005D050(&h));
        }
    }
}

/* Creates the effect record the eft022 setter leaves behind. */
extern "C" void fn_80115A44(void** out)
{
    fn_800E3B2C();
    *out = lbl_805A0360;
}

/* Frees an eft022 record once the effect is gone. */
extern "C" void fn_801164F0(_EFT* self)
{
    _EFT_WORK_C* work = (_EFT_WORK_C*)self->work_0x38;

    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* Runs the state handler the eft022 record's `state_0x05` selects. */
extern "C" void fn_8011652C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_80116568(self);
    case 1:
        return fn_8011661C(self);
    case 2:
        return fn_8011675C(self);
    case 3:
        return fn_8011676C(self);
    }
}

/* Spawns the record's effect and hands its position to the enemy it belongs to. */
extern "C" void fn_80116568(_EFT* self)
{
    _EFT_WORK_C* work = (_EFT_WORK_C*)self->work_0x38;
    _ENEMY_WORK* enemy;

    self->state_0x05++;
    work->effects[0] = res_eft_create(0x2D, 7, 0);
    if (work->effects[0] == 0) {
        fn_8011676C(self);
        return;
    }
    SetRootMtxTrans(work->effects[0], &self->pos_0x18);
    fn_8011661C(self);
    if (self->type_0x02 != 1) {
        fn_800DA864(&self->pos_0x18);
        return;
    }
    enemy = (_ENEMY_WORK*)self->source_0x30;
    if (em_work_die_ck(enemy) == 0) {
        fn_800DCB18(enemy->field_0xB14, &self->pos_0x18);
    }
}

/* Advances the eft022 record's timer through its four substates. */
extern "C" void fn_8011661C(_EFT* self)
{
    _EFT_WORK_C* work = (_EFT_WORK_C*)self->work_0x38;

    if (effect_move(work->effects[0]) == 1) {
        fn_800F93D8_c1(self, &work->effects[0], 1, work->count, 0);
    }
    switch (self->field_0x07) {
    case 0:
        if (++self->timer_0x0C >= 0x18) {
            self->timer_0x0C = 5;
            fn_80059374(1);
            self->field_0x07 = (u8)((u8)(self->field_0x07 + 1) + 1);
        }
        break;
    case 1:
        self->field_0x07 = (u8)(self->field_0x07 + 1);
        break;
    case 2:
        if (--self->timer_0x0C > 0) {
            fn_80059420();
        } else {
            self->timer_0x0C = 0x5A;
            fn_80059374(2);
            self->field_0x07 = (u8)(self->field_0x07 + 1);
        }
        break;
    case 3:
        if (--self->timer_0x0C > 0) {
            fn_80059420();
        } else {
            self->flag_0x01 = 0;
            self->state_0x05++;
            self->field_0x06 = 0;
            self->field_0x07 = 0;
        }
        break;
    }
}

/* Bumps the eft022 record's `state_0x05` state index. */
extern "C" void fn_8011675C(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the eft022 record: the family-22 state-3 step. */
extern "C" void fn_8011676C(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Creates the effect's eft022 record at the current area. */
void eft022_set(nw4r::math::VEC3* pos, u8 area)
{
    _EFT* effect;
    _EFT_WORK_C* work;

    if ((u8)area != (u8)get_now_areano()) {
        return;
    }
    effect = fn_800F8788_c1(8);
    if (effect == 0) {
        return;
    }
    work = (_EFT_WORK_C*)effect->work_0x38;
    work->count = 1;
    effect->release_0x40 = fn_801164F0;
    effect->field_0x03 = 22;
    effect->area_0x44 = area;
    copyVec3(&effect->pos_0x18, pos);
    effect->type_0x02 = 0;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    fn_800F9DF4_c1(effect, 0, 0);
    effect->dispatch_0x34 = fn_8011652C;
}

/* Creates the effect's eft022 record for an actor. */
extern "C" void fn_80116430(void* source, nw4r::math::VEC3* pos, u8 area)
{
    _EFT* effect;
    _EFT_WORK_C* work;

    if ((u8)area != (u8)get_now_areano()) {
        return;
    }
    effect = fn_800F8788_c1(8);
    if (effect == 0) {
        return;
    }
    work = (_EFT_WORK_C*)effect->work_0x38;
    work->count = 1;
    effect->release_0x40 = fn_801164F0;
    effect->field_0x03 = 22;
    effect->area_0x44 = area;
    copyVec3(&effect->pos_0x18, pos);
    effect->type_0x02 = 1;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    effect->source_0x30 = source;
    fn_800F9DF4_c1(effect, 0, 0);
    effect->dispatch_0x34 = fn_8011652C;
}

/* ---------------------------------------------------------------------------------------------------
 * eft023 (fn_8011688C release, fn_8011689C dispatch)
 * --------------------------------------------------------------------------------------------------- */

/* Creates the eft023 record and its per-area model set. */
extern "C" void fn_80116770(void)
{
    _EFT* effect;
    _EFT_JOB_WORK* work;
    s32 i;
    u8 map;
    s32 id;

    effect = fn_800F8788_c1(0x4C);
    if (effect == 0) {
        return;
    }
    effect->release_0x40 = fn_8011688C;
    work = (_EFT_JOB_WORK*)effect->work_0x38;
    work->count = 1;
    effect->area_0x44 = (u8)get_now_areano();
    map = get_now_mapno();
    if (map == 6 || map == 0x11) {
        if (effect->area_0x44 == 1) {
            work->count = 5;
        } else if (effect->area_0x44 == 2) {
            work->count = 2;
        }
    }
    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get_c1();
        if (work->models[i] == 0) {
            eft_res_slot_release(effect);
            return;
        }
    }
    effect->source_0x30 = 0;
    effect->dispatch_0x34 = fn_8011689C;
    effect->field_0x03 = 23;
    fn_800F9DF4_c1(effect, 8, 0);
}

/* Releases the eft023 record's character handles. */
extern "C" void fn_8011688C(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;

    fn_800F8A44((void**)work->models, work->count);
}

/* Runs the state handler the eft023 record's `state_0x05` selects. */
extern "C" void fn_8011689C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_801168D8(self);
    case 1:
        return fn_80116B00(self);
    case 2:
        return fn_80116D90(self);
    case 3:
        return fn_80116DA0(self);
    }
}

/* Spawns the eft023 record's models for the current map and area. */
extern "C" void fn_801168D8(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;
    s32 i;
    u8 map;
    s32 id;

    self->state_0x05++;
    map = get_now_mapno();
    switch (map) {
    case 1:
        id = 0x11;
        break;
    case 12:
        id = 0x12;
        break;
    case 6:
        id = (self->area_0x44 == 1) ? 0x13 : 0x15;
        break;
    case 17:
        id = 0x16;
        if (self->area_0x44 == 1) {
            id = 0x14;
        }
        break;
    }
    for (i = 0; i < work->count; i++) {
        switch (get_now_mapno()) {
        case 1:
        case 12:
            if (self->area_0x44 == 1) {
                work->tables[i] = &lbl_805A0380[i];
            }
            break;
        case 6:
        case 17:
            if (self->area_0x44 == 1) {
                work->tables[i] = &lbl_805A039C[i];
            } else if (self->area_0x44 == 2) {
                work->tables[i] = &lbl_805A0428[i];
            }
            break;
        }
        if (res_eft_model_create_light(work->models[i], id, 0x28, 2) == 0) {
            fn_80116DA0(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    self->field_0x10 = 0;
    for (i = 0; i < work->count; i++) {
        vec_to_mh_vec3(&work->models[i]->pos_0x04, (Vec*)&work->tables[i]->pos_0x04);
        setVector3(&work->models[i]->scale_0x1C, lbl_80796A60, lbl_80796A60,
                                           lbl_80796A60);
        work->models[i]->field_0x28 = 0;
        work->models[i]->field_0x2C = work->tables[i]->field_0x12;
        work->models[i]->field_0x28 = 0;
        work->models[i]->ready_0x35 = 0;
        work->field_0x18[i] = 0;
        work->field_0x22[i] = (s16)(work->tables[i]->field_0x10 - 1);
        work->field_0x2C[i] = 0;
    }
    fn_80116B00(self);
}

/* Bumps the eft023 record's `state_0x05` state index. */
extern "C" void fn_80116D90(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the eft023 record: the family-23 state-3 step. */
extern "C" void fn_80116DA0(_EFT* self)
{
    eft_res_slot_release(self);
}

/* ---------------------------------------------------------------------------------------------------
 * eft024 (fn_80116FCC release, fn_80116FDC dispatch)
 * --------------------------------------------------------------------------------------------------- */

/* Creates the eft024 job record. */
extern "C" void fn_80116DA4(void)
{
    _EFT* effect = fn_800F8788_c1(0x84);

    if (effect != 0) {
        effect->source_0x30 = 0;
        effect->dispatch_0x34 = fn_80116FDC;
        effect->field_0x03 = 24;
        fn_800F9DF4_c1(effect, 8, 0);
        effect->flag_0x01 = 1;
        effect->area_0x44 = (u8)get_now_areano();
    }
}

/* Allocates one eft024 character slot. */
extern "C" void* fn_80116E14(s16 unused)
{
    return fn_800F8788_c1(0x20);
}

/* Adds one character to the eft024 job's free slot. */
extern "C" void fn_80116E1C(_EFT* self, s16 type)
{
    _EFT_JOB2_WORK* work = (_EFT_JOB2_WORK*)self->work_0x38;
    s16 slot;
    _EFT* effect;

    slot = fn_80116EEC(self);
    if (slot == -1) {
        return;
    }
    effect = (_EFT*)fn_80116E14(type);
    if (effect == 0) {
        return;
    }
    effect->field_0x03 = 24;
    effect->type_0x02 = (u8)type;
    effect->dispatch_0x34 = fn_80116FDC;
    fn_800F9DF4_c1(effect, 8, 0);
    effect->area_0x44 = (u8)get_now_areano();
    work->slots[slot] = effect;
    work->per_type[type - 1]++;
    work->total++;
}

/* Finds the first free slot in the eft024 job's 32-slot table. */
extern "C" s16 fn_80116EEC(_EFT* self)
{
    s16 i = 0;
    _EFT** p = (_EFT**)self->work_0x38;
    s32 j;
    s32 k;

    for (j = 0; j < 4; j++) {
        for (k = 0; k < 8; k++) {
            if (p[k] == 0) {
                return i;
            }
            i++;
        }
        p += 8;
    }
    return -1;
}

/* Releases the eft024 job's character handles. */
extern "C" void fn_80116FCC(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;

    fn_800F8A44((void**)work->models, work->count);
}

/* Runs the state handler the eft024 job's `state_0x05` selects. */
extern "C" void fn_80116FDC(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        return fn_80117074(self);
    case 3:
        return fn_80117084(self);
    default:
        return;
    }
    switch (self->state_0x05) {
    case 0:
        self->state_0x05++;
        self->flag_0x01 = 1;
        switch (self->type_0x02) {
        case 0:
            return fn_80117088(self);
        case 1:
            return fn_8011722C(self);
        }
        return;
    case 1:
        switch (self->type_0x02) {
        case 0:
            return fn_80117120(self);
        case 1:
            return fn_801173AC(self);
        }
        return;
    }
}

/* Bumps the eft024 job's `state_0x05` state index. */
extern "C" void fn_80117074(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the eft024 job: the family-24 state-3 step. */
extern "C" void fn_80117084(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Zeroes the eft024 job's slot table and counters. */
extern "C" void fn_80117088(_EFT* self)
{
    _EFT_JOB2_WORK* work = (_EFT_JOB2_WORK*)self->work_0x38;
    s32 i;

    self->timer_0x0C = 0;
    for (i = 0; i < 32; i++) {
        work->slots[i] = 0;
    }
    work->total = 0;
    work->per_type[0] = 0;
}

#pragma unroll off

/* Spawns a new character every second while the eft024 job's table has room, and reaps dead ones. */
extern "C" void fn_80117120(_EFT* self)
{
    _EFT_JOB2_WORK* work = (_EFT_JOB2_WORK*)self->work_0x38;
    _EFT* handle;
    s32 i;

    if (++self->timer_0x0C >= 0x3C && work->total < 0x20) {
        fn_80116E1C(self, 1);
        self->timer_0x0C = 0;
    }
    for (i = 0; i < 0x20; i++) {
        handle = work->slots[i];
        if (handle != 0 && handle->state_0x05 >= 2) {
            work->per_type[handle->type_0x02 - 1]--;
            work->slots[i] = 0;
            work->total--;
        }
    }
}

#pragma unroll reset

/* Moves the eft021 record's models and tints them from the shared key tables. */
extern "C" void fn_801156A0(_EFT* self)
{
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    _EFT_WORK_B* work;
    _PLW* plw;
    _EFT_MODEL_OBJ* obj;
    _GXColor color;
    s32 i;
    s32 j;
    f32 z;

    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    work = (_EFT_WORK_B*)self->work_0x38;
    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->models[i], 8, 0xC) == 0) {
            fn_80115FB0(self);
            return;
        }
    }
    switch (self->type_0x02) {
    case 0:
        plw = (_PLW*)self->source_0x30;
        setVisibility__6MHcharFUlb(work->models[0], 1, 1);
        setVisibility__6MHcharFUlb(work->models[0], 2, 0);
        fn_800E0A14(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[0][0]);
        self->field_0x24 = (s32)plw->param_0x54;
        self->field_0x28 = (s32)plw->field_0x058;
        self->field_0x2C = (s32)plw->rot_z_0x5C;
        break;
    case 1:
    case 3:
    case 4:
        plw = (_PLW*)self->source_0x30;
        setVisibility__6MHcharFUlb(work->models[0], 1, 0);
        setVisibility__6MHcharFUlb(work->models[0], 2, 1);
        fn_800E0A14(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[0][0]);
        self->field_0x24 = (s32)plw->param_0x54;
        self->field_0x28 = (s32)plw->field_0x058;
        self->field_0x2C = (s32)plw->rot_z_0x5C;
        break;
    default:
        fn_80115FB0(self);
        return;
    }
    for (i = 0; i < work->count; i++) {
        obj = (_EFT_MODEL_OBJ*)operator new(0xC);
        if (obj != 0) {
            fn_80115A44((void**)&obj);
        }
        if (obj == 0) {
            fn_80115FB0(self);
            return;
        }
        work->objects[i] = obj;
        obj->owner_0x04 = self;
        obj->index_0x08 = i;
        fn_800E25B0((void*)work->models[i]->field_0x118, obj);
        (*(void (*)(void*))obj->vtable[6])(obj);
        work->models[i]->field_0x40 = 0;
        work->joint_num = get_joint_num__6MHcharFv(work->models[i]);
        vec_to_mh_vec3(&v, (Vec*)&lbl_805A0208[self->type_0x02]);
        if (i == 1) {
            v.x *= lbl_80796A30;
        }
        mulVecMat(&v, &work->mtx[i][0]);
        z = work->mtx[i][0].m[2][3];
        work->mtx[i][0].m[0][3] += v.x;
        work->mtx[i][0].m[1][3] += v.y;
        work->mtx[i][0].m[2][3] = z + v.z;
        mtx34_trans_get_c1(&work->mtx[i][0], &self->pos_0x18, z);
        copyVec3(&work->models[i]->pos_0x04, &self->pos_0x18);
        work->models[i]->field_0x28 = self->field_0x24;
        work->models[i]->field_0x2C = self->field_0x28;
        work->models[i]->field_0x30 = self->field_0x2C;
        color = lbl_805A0258[self->type_0x02];
        switch (self->type_0x02) {
        case 0:
            setMatColor__6MHcharFUl12_GXChannelID8_GXColorb(work->models[i], 0, GX_COLOR0A0, color, 0);
            break;
        case 1:
        case 3:
        case 4:
            setMatColor__6MHcharFUl12_GXChannelID8_GXColorb(work->models[i], 1, GX_COLOR0A0, color, 0);
            break;
        }
        for (j = 0; j < 5; j++) {
            mtx34_copy(&work->mtx[i][j + 1], &work->mtx[i][j]);
        }
    }
}

/* Advances the eft023 record's per-model joint visibility and rotation. */
extern "C" void fn_80116B00(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;
    _EFT_MAP_TABLE* table;
    s32 i;
    u32 j;
    u32 joint_num;
    f32 f;
    s16 t;
    u8 base;
    f32 f25 = lbl_80796A78;
    f32 f26 = lbl_80796A64;
    f32 f27 = lbl_80796A68;
    f32 f28 = lbl_80796A60;
    f32 f29 = lbl_80796A6C;
    f32 f30 = lbl_80796A74;
    f32 f31 = lbl_80796A70;

    for (i = 0; i < work->count; i++) {
        table = work->tables[i];
        work->field_0x22[i] = (s16)(work->field_0x22[i] + 1);
        base = table->field_0x10;
        if (work->field_0x22[i] >= (s32)(base + table->field_0x11)) {
            work->field_0x22[i] = base;
        }
        joint_num = get_joint_num__6MHcharFv(work->models[i]);
        for (j = 0; j < joint_num; j++) {
            if ((s32)j == work->field_0x22[i]) {
                setVisibility__6MHcharFUlb(work->models[i], j, 1);
            } else {
                setVisibility__6MHcharFUlb(work->models[i], j, 0);
            }
        }
        self->field_0x10++;
        work->field_0x18[i] = (s16)(work->field_0x18[i] + 1);
        t = work->field_0x18[i];
        f = fn_80463EE0(t, (f26 * (f32)(u16)(t << table->field_0x18)) / f27);
        if ((f >= f28 || f <= f29) && table->field_0x1A > 0) {
            work->field_0x2C[i] = (s16)(work->field_0x2C[i] + 1);
            if (work->field_0x2C[i] > table->field_0x1A) {
                work->field_0x2C[i] = 0;
            } else {
                work->field_0x18[i] = (s16)(work->field_0x18[i] - 1);
            }
        }
        work->models[i]->field_0x2C =
            (s32)(u16)(s32)(f31 + ((f27 * (f * table->field_0x14)) / f30)) + table->field_0x12;
        move__6MHcharFUs(work->models[i], 0);
        fn_800F93D8_c1(self, &work->models[i], 2, 1, 0);
    }
}

/* Moves the eft021 record's models through their pose animation. */
extern "C" void fn_80115A80(_EFT* self)
{
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    _EFT_WORK_B* work;
    _PLW* plw;
    _GXColor color;
    s32 i;
    s32 j;
    s32 moved;
    u32 motion;
    s16 t;
    s8 r;
    f32 f0;
    f32 f1;
    f32 f25 = lbl_80796A30;
    f32 f27 = lbl_80796A38;
    f32 f28 = lbl_80796A3C;
    f32 f29 = lbl_80796A40;
    f32 f30 = lbl_80796A44;
    f32 f31 = lbl_80796A48;

    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    work = (_EFT_WORK_B*)self->work_0x38;
    if (self->type_0x02 != 2) {
        plw = (_PLW*)self->source_0x30;
        if (plw != 0) {
            self->type_0x02 = plw->field_0x65E;
            self->area_0x44 = plw->area_0x16;
        }
    }
    self->timer_0x0C = 0;
    for (i = 0; i < work->count; i++) {
        if ((u32)(self->type_0x02 - 3) <= 1U) {
            if (fn_800F92F4_c1(self, 0) == 0) {
                self->flag_0x01 = 0;
                self->state_0x05++;
                return;
            }
            plw = (_PLW*)self->source_0x30;
            r = fn_802748C8(plw);
            if (r == 0) {
                t = plw->field_0x384;
                if (t < 0x3C) {
                    f0 = lbl_80796A34;
                } else {
                    f0 = (f32)(t - 0x3C) / f27;
                }
                if (f0 > f28) {
                    f0 = f28;
                }
                color.r = 0xFF;
                color.g = 0xFF - (s32)(f29 * f0);
                color.b = 0xFF - (s32)(f29 * f0);
                if (t < 0x14) {
                    f1 = lbl_80796A34;
                } else {
                    f1 = (f32)(t - 0x14) / f30;
                }
                if (f1 > f28) {
                    f1 = f28;
                }
                color.a = (s8)(f31 * f1);
            } else {
                color = lbl_805A026C[(s8)(r - 1)];
            }
            setMatColor__6MHcharFUl12_GXChannelID8_GXColorb(work->models[i], 1, GX_COLOR0A0, color, 0);
        }
        if (fn_800F92F4_c1(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        plw = (_PLW*)self->source_0x30;
        if ((u32)(plw->field_0x65E - 3) > 1U) {
            switch (plw->field_0x65E) {
            case 0:
                setVisibility__6MHcharFUlb(work->models[i], 1, 1);
                setVisibility__6MHcharFUlb(work->models[i], 2, 0);
                break;
            default:
                fn_80115FB0(self);
                return;
            }
        } else {
            setVisibility__6MHcharFUlb(work->models[i], 1, 0);
            setVisibility__6MHcharFUlb(work->models[i], 2, 1);
        }
        motion = Get_motion_no(plw);
        if (plw->field_0x662 <= 0 || (plw->field_0x664 != motion && plw->field_0x666 != motion)) {
            moved = 0;
            fn_800E0A14(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[i][0]);
            for (j = 0; j < 5; j++) {
                mtx34_copy(&work->mtx[i][j + 1], &work->mtx[i][j]);
            }
        } else {
            moved = 1;
        }
        for (j = 5; j > 0; j--) {
            mtx34_copy(&work->mtx[i][j], &work->mtx[i][j - 1]);
        }
        fn_800E0A14(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[i][0]);
        self->flag_0x01 = plw->field_0x001;
        self->field_0x24 = (s32)plw->param_0x54;
        self->field_0x28 = (s32)plw->field_0x058;
        self->field_0x2C = (s32)plw->rot_z_0x5C;
        vec_to_mh_vec3(&v, (Vec*)&lbl_805A0208[self->type_0x02]);
        if (i == 1) {
            v.x *= f25;
        }
        mulVecMat(&v, &work->mtx[i][0]);
        f1 = work->mtx[i][0].m[2][3];
        work->mtx[i][0].m[0][3] += v.x;
        work->mtx[i][0].m[1][3] += v.y;
        work->mtx[i][0].m[2][3] = f1 + v.z;
        mtx34_trans_get_c1(&work->mtx[i][0], &self->pos_0x18, f1);
        copyVec3(&work->models[i]->pos_0x04, &self->pos_0x18);
        work->models[i]->field_0x28 = self->field_0x24;
        work->models[i]->field_0x2C = self->field_0x28;
        work->models[i]->field_0x30 = self->field_0x2C;
        move2__6MHcharFPQ34nw4r4math5MTX34Us(work->models[i], &work->mtx[i][0], 0);
        if (moved == 1U) {
            fn_800F93D8_c1(self, &work->models[i], 2, 1, 0);
        }
    }
}

/* Places one eft021 model's pose from the shared offset tables. */
extern "C" void fn_80116080(_EFT_MODEL_OBJ* obj, nw4r::math::MTX34* mtx_arr, s32 arg2)
{
    nw4r::math::MTX34 out;
    nw4r::math::MTX34 ma;
    nw4r::math::MTX34 mb;
    nw4r::math::MTX34 mc;
    nw4r::math::MTX34* dst;
    nw4r::math::VEC3* a;
    nw4r::math::VEC3* b;
    _EFT* effect;
    _EFT_WORK_B* work;
    nw4r::math::MTX34* src;
    s32 h0;
    s32 h1;
    s32 id0;
    s32 id1;
    s32 k;
    s32 group;
    s32 inner;
    f32 f31 = lbl_80796A58;
    f32 f5;
    f32 f4;
    f32 f0;

    effect = obj->owner_0x04;
    work = (_EFT_WORK_B*)effect->work_0x38;
    VEC3_ctor((nw4r::math::VEC3*)&out);
    VEC3_ctor((nw4r::math::VEC3*)&ma);
    VEC3_ctor((nw4r::math::VEC3*)&mb);
    MTX34_ctor(&mc);
    MTX34_ctor(&out);
    MTX34_ctor(&ma);
    MTX34_ctor(&mb);
    fn_8005D1AC(&h0, 0);
    fn_8005D1AC(&h1, 0);
    src = &work->mtx[obj->index_0x08][0];
    id0 = fn_80097F18(arg2, 3U);
    fn_8005D0CC(&h0, &id0);
    copyMat33(&mtx_arr[fn_8006FDCC(&h0)], src);
    k = 4;
    for (group = 0; group < 4; group++) {
        mtx34_copy(&ma, &src[group]);
        mtx34_copy(&mb, &src[group + 1]);
        mtx34_copy(&mc, &src[group + 2]);
        for (inner = 0; inner < 6; inner++) {
            a = lbl_805A0330[inner];
            b = lbl_805A0300[inner];
            id1 = fn_80097F18(arg2, k);
            fn_8005D0CC(&h1, &id1);
            dst = &mtx_arr[fn_8006FDCC(&h1)];
            fn_80051894(&out, &ma, &mb, 1, a->x, a->y);
            out.m[0][3] *= lbl_805A0244[effect->type_0x02];
            out.m[1][3] *= lbl_805A0244[effect->type_0x02];
            out.m[2][3] *= lbl_805A0244[effect->type_0x02];
            f5 = b->z * ((mb.m[0][3] + mc.m[0][3]) * f31) +
                 (b->x * ((ma.m[0][3] + mb.m[0][3]) * f31) + mb.m[0][3] * b->y);
            f4 = b->z * ((mb.m[1][3] + mc.m[1][3]) * f31) +
                 (b->x * ((ma.m[1][3] + mb.m[1][3]) * f31) + mb.m[1][3] * b->y);
            f0 = b->z * ((mb.m[2][3] + mc.m[2][3]) * f31) +
                 (b->x * ((ma.m[2][3] + mb.m[2][3]) * f31) + mb.m[2][3] * b->y);
            out.m[0][3] = f5;
            out.m[1][3] = f4;
            out.m[2][3] = f0;
            mtx34_copy(dst, &out);
            k++;
            if (k >= 0x1B) {
                return;
            }
        }
    }
}
