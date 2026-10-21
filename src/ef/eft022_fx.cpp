/* ef/eft022_fx.cpp - the eft021 and eft022 effect families.  Each creator takes the 0x48-byte `_EFT` record with a work
 *   block from `eft_res_slot_get`, stamps `+0x03` and installs its hooks: 21 `fn_80115460` (0x25C `_EFT_WORK_B`,
 *   release `fn_80115554`, dispatch `fn_80115664`), 22 `eft022_set` (0x8 `_EFT_WORK_C`, `fn_801164F0`, `fn_8011652C`).
 * RANGE. .text 0x801153D0-0x80116770 (20 functions); extab 0x8000C3F4-0x8000C464, extabindex 0x800264D8-0x80026580,
 *   .data 0x805A0208-0x805A0380, .sdata 0x80791930-0x80791938, .sdata2 0x80796A30-0x80796A60.  Family 20 below
 *   0x801153D0 is `ef/fn_80114E34.cpp`; the eft023/eft024 job machines from 0x80116770 are `ef/eft023_fx.cpp` (their
 *   own `.sdata2` pool repeats this one's constants).
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (playbook 39).
 * NAMES. `eft022_set` is the runtime dump's own name; the map has only `fn_` stems for the rest, so plain definitions
 *   are `extern "C"`.
 *   GUESS: `EftModelObjRelease::destroy` (the model object's deleting destructor slot, `fn_80115608` in the table at
 *   0x805A0360).
 * RESIDUALS.
 *  - `fn_80116080`: the pose-blend loop's float scheduling;
 *  - `fn_80115A80`, `fn_801156A0`: register colouring and pool-order differences.
 *   `.data`/`.sdata` are still declared, not emitted.
 *   Relocation names that differ from retail: pool constants, save helpers, `fn_8050131C`/`MTX34Zero`.
 * SHAPES. Functions are defined in retail's address order.  u8 area stores take `get_now_areano()` uncast (a `(u8)`
 *   cast keeps a `clrlwi` under peephole off); type tests that retail compares signed are `(s32)` casts.
 *   `fn_8011661C` bumps its phase twice (`field_0x07++; field_0x07++;`: one `clrlwi` between the adds) and tests
 *   `--timer <= 0` first.
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
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
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

/* The model object's virtual interface as the release path sees it: the first slot (+0x08 in the table at
 * 0x805A0360) is the deleting destructor `fn_80115608`.  Declared only, so this unit emits no table for it.
 * size: 0x04 */
class EftModelObjRelease {
public:
    virtual void destroy(s32 dealloc);
};

/* The eft021 work block. size: 0x25C */
struct _EFT_WORK_B {
    /* +0x000 */ s32 count;
    /* +0x004 */ MHchar* models[2];
    /* +0x00C */ nw4r::math::MTX34 mtx[2][6];
    /* +0x24C */ void* objects[2];
    /* +0x254 */ s32 joint_num;
    /* +0x258 */ u16 field_0x258;
    /* +0x25A */ u16 field_0x25A;
};

/* The eft022 work block. size: 0x08 */
struct _EFT_WORK_C {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
};

/* size: 0x148 (lower bound) */

extern "C" void* fn_801153D0(void*, s32, u8);
extern "C" void* fn_80115460(u8);
extern "C" void fn_80115554(_EFT*);
extern "C" s32 fn_80115608(void*, s16);
extern "C" void fn_80115664(_EFT*);
extern "C" void fn_801156A0(_EFT*);
extern "C" _EFT_MODEL_OBJ* fn_80115A44(void**);
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

/* The library entry points, declared by their map spelling (a mangled callee is not evidence of a
 * C++ unit; only a mangled definition is). */


extern "C" void mtx34_copy(void* dst, void* src);

extern "C" void fn_80059374(s32 a);
extern "C" void fn_80059420(void);
extern "C" void res_node_assign(void* ctx, void* val);
extern "C" s32 res_node_get_id(void* ctx);
extern "C" u32 res_node_is_valid(void* ctx);
extern "C" void fn_8050131C(void* a);
extern "C" void fn_80051894(void* a, void* b, void* c, s32 d, f32 e, f32 f);

extern "C" u32 get_joint_num__6MHcharFv(void* chr);
extern "C" void setVisibility__6MHcharFUlb(void* chr, u32 joint, u32 visible);
extern "C" void setMatColor__6MHcharFUl12_GXChannelID8_GXColorb(void* chr, u32 idx,
                                                                _GXChannelID channel,
                                                                _GXColor color, u32 keep);
extern "C" void move2__6MHcharFPQ34nw4r4math5MTX34Us(void* chr, nw4r::math::MTX34* m, u16 a);

extern "C" nw4r::math::VEC3 lbl_805A0208[];
extern "C" f32 lbl_805A0244[];
extern "C" _GXColor lbl_805A0258[];
extern "C" _GXColor lbl_805A026C[];
extern "C" nw4r::math::VEC3* lbl_805A0300[];
extern "C" nw4r::math::VEC3* lbl_805A0330[];
extern "C" u8 lbl_805A0360[];

extern "C" char lbl_80791930[8];


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
    work->field_0x25A = 0xFFFF;
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
    s32 i;
    s32 j;
    _EFT_WORK_B* work;
    _EFT_MODEL_OBJ* object;

    work = (_EFT_WORK_B*)self->work_0x38;
    eft_res_models_release((void**)work->models, work->count);
    for (i = 0; i < work->count; i++) {
        for (j = 0; j < 6; j++) {
            fn_8050131C(&work->mtx[i][j]);
        }
        object = (_EFT_MODEL_OBJ*)work->objects[i];
        if (object != 0) {
            ((EftModelObjRelease*)object)->destroy(1);
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
        mhchar_joint_mtx_get(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[0][0]);
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
        mhchar_joint_mtx_get(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[0][0]);
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
            v.x *= -1.0f;
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

/* Creates the effect record the eft022 setter leaves behind. */
extern "C" _EFT_MODEL_OBJ* fn_80115A44(void** out)
{
    fn_800E3B2C();
    *out = lbl_805A0360;
    return (_EFT_MODEL_OBJ*)out;
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
    f32 f25 = -1.0f;
    f32 f27 = 60.0f;
    f32 f28 = 1.0f;
    f32 f29 = 204.0f;
    f32 f30 = 20.0f;
    f32 f31 = 255.0f;

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
                    f0 = 0.0f;
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
                    f1 = 0.0f;
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
            mhchar_joint_mtx_get(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[i][0]);
            for (j = 0; j < 5; j++) {
                mtx34_copy(&work->mtx[i][j + 1], &work->mtx[i][j]);
            }
        } else {
            moved = 1;
        }
        for (j = 5; j > 0; j--) {
            mtx34_copy(&work->mtx[i][j], &work->mtx[i][j - 1]);
        }
        mhchar_joint_mtx_get(&plw->physics_0x13C->chr_0x04, 7, &work->mtx[i][0]);
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
    nw4r::g3d::ResNode h((void*)0);
    _EFT* effect = obj->owner_0x04;
    s32 id;
    s32 i;
    _EFT_WORK_B* work = (_EFT_WORK_B*)effect->work_0x38;

    for (i = 0; i < work->count; i++) {
        s32 handle = work->models[i]->field_0x118;
        fn_80080B10((void*)handle, 4);   /* the declaration takes void*, as its C sibling unit casts */
        id = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(&work->models[i]->field_0x114)->GetResNode((const char*)lbl_80791930).mpData;
        res_node_assign(&h, &id);
        if (res_node_is_valid(&h) == 1U) {
            fn_800E3264((void*)handle, res_node_get_id(&h));
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
    s32 k;
    s32 group;
    s32 inner;
    f32 f31 = 0.5f;
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
    nw4r::g3d::ResNode h0((void*)0);
    nw4r::g3d::ResNode h1((void*)0);
    s32 id0;
    s32 id1;
    src = &work->mtx[obj->index_0x08][0];
    id0 = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(arg2)->GetResNode((u32)3).mpData;
    res_node_assign(&h0, &id0);
    copyMat33(&mtx_arr[fn_8006FDCC(&h0)], src);
    k = 4;
    for (group = 0; group < 4; group++) {
        mtx34_copy(&ma, &src[group]);
        mtx34_copy(&mb, &src[group + 1]);
        mtx34_copy(&mc, &src[group + 2]);
        for (inner = 0; inner < 6; inner++) {
            a = lbl_805A0330[inner];
            b = lbl_805A0300[inner];
            id1 = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(arg2)->GetResNode((u32)k).mpData;
            res_node_assign(&h1, &id1);
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
    if ((s32)self->type_0x02 != 1) {
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
            self->field_0x07++;
            self->field_0x07++;
        }
        break;
    case 1:
        self->field_0x07++;
        break;
    case 2:
        if (--self->timer_0x0C <= 0) {
            self->timer_0x0C = 0x5A;
            fn_80059374(2);
            self->field_0x07++;
        } else {
            fn_80059420();
        }
        break;
    case 3:
        if (--self->timer_0x0C <= 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            self->field_0x06 = 0;
            self->field_0x07 = 0;
        } else {
            fn_80059420();
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
