/* ef/fn_801173AC.cpp - the eft024 job machine's kind-1 state-1 handler (the sibling of `ef/fn_8011722C.c`'s state-0
 *   entry: it integrates the spin deltas, pushes the transform onto the pooled `MHchar` and advances the job when the
 *   model falls below the ground) and the whole eft025 player family (two setters, the allocator stamping tag 25,
 *   the release, the dispatcher and its four states).
 * RANGE. .text 0x801173AC-0x80117DA8 (10 functions); extab 0x8000C49C-0x8000C4D4, extabindex 0x800265D4-0x80026628,
 *   .sdata 0x80791938-0x80791940, .sdata2 0x80796A88-0x80796AC0.  The eft026/eft028 families from 0x80117DA8 are
 *   `ef/eft026_fx.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (playbook 39).
 * NAMES. The map has only `fn_` stems here, so the definitions are `extern "C"` in a C++ unit.
 * RESIDUALS. 5 partial rows: `fn_801173AC`, `fn_80117688`, `fn_80117760` scheduling and frame layout; `fn_80117894`
 *   addresses `lbl_80791938` with `lis`/`addi` where retail uses `@sda21`; `fn_80117A1C` calls `event_demo_ck` by its
 *   plain name where retail calls `event_demo_ck__Fv`.
 *   flipcheck: `.sdata` claimed, not emitted; `.text` 0xA14 against the claimed 0x9FC, `.sdata2` 0x8 of 0x38, both
 *   differing; extabindex differs in 3 bytes; `event_demo_ck` has no map row.
 * SHAPES. `get_camera_pos`/`get_camera_direction` are declared with C++ linkage (their map names are `__Fv`).
 *   The `MHchar`/`Effect` vtable slots are reached through function-pointer tables (retail's `lwz r12, off(r12)`
 *   shape; a `virtual` declaration would emit the class's vtable into this object).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "unsplit/sound.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "nw4r/g3d/scnmdl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/fn_801173AC_types.h"

/* The eft024 job's kind-1 work view (`fn_80114E34.cpp`'s family-24 `job->work_0x38` block, 0x84 B).
 * `ef/fn_8011722C.c` is the kind-1 state-0 sibling and names the same offsets. size: 0x20 (lower bound,
 * the highest offset this file reads). */
typedef struct _EFT24_CHARA {
    /* +0x00 */ s32 count;        /* live character handles, always 1 in the kind-1 slot */
    /* +0x04 */ MHchar* chara[1]; /* the pooled handles */
    /* +0x08 */ VEC3 pos;         /* per-frame position delta, integrated by fn_801173AC */
    /* +0x14 */ u16 rot_x;        /* rotation angles pushed to the model */
    /* +0x16 */ u16 rot_y;
    /* +0x18 */ u16 rot_z;
    /* +0x1A */ s16 spin_x;       /* per-frame spin deltas, decayed then integrated */
    /* +0x1C */ s16 spin_y;
    /* +0x1E */ s16 spin_z;
} _EFT24_CHARA;

/* The eft025 kind-3 source: its `MHchar` sits at +8 (the view `ef/fn_80114E34.cpp` states for the same
 * record). size: 0x148 (lower bound). */
typedef struct _EFT25_CHARA_SRC {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ MHchar chr_0x08;
} _EFT25_CHARA_SRC;

/* The eft025 work block `eft_res_slot_get(0x10)` attaches. size: 0x10 */
typedef struct _EFT25_WORK {
    /* +0x00 */ s32 count;        /* pooled models, always 1 */
    /* +0x04 */ void* models[1];  /* the pooled `nw4r::ef::Effect` handles */
    /* +0x08 */ s32 motion;       /* the source's motion number latched in state 0, type 2 */
    /* +0x0C */ f32 scale;        /* the per-record scale fn_801176F0 carries */
} _EFT25_WORK;

extern "C" {
/* this unit's own symbols (definitions below; the map's plain stems) */
void fn_801173AC(_EFT* self);
void fn_80117688(_PLW* plw, u8 kind);
void fn_801176F0(_PLW* plw, f32 value);
_EFT* fn_80117760(u8 kind, u8 area);
void fn_8011781C(_EFT* self);
void fn_80117858(_EFT* self);
void fn_80117894(_EFT* self);
void fn_80117A1C(_EFT* self);
void fn_80117D94(_EFT* self);
void fn_80117DA4(_EFT* self);

/* eft_res_model_get comes from the owner's header `ef/eft_res.h` (rule 2): this unit's local
 * `void*` copy collided with the owner's `u8*` definition once the header declared it. */
s32 eft_res_spawn_gate_ck(_EFT* self, u32 flag);
void fn_800E0A14(void* mhchar, u32 joint, Mtx34* out);
u32 event_demo_ck(void);

extern u16 lbl_80791938[];

extern f32 lbl_80796A88;
extern f32 lbl_80796A8C;
extern f32 lbl_80796A90;
extern f32 lbl_80796A94;
extern f32 lbl_80796A98;
extern f32 lbl_80796A9C;
extern f32 lbl_80796AA0;

extern f32 lbl_80796AB0;
extern f32 lbl_80796AB4;
extern f32 lbl_80796AB8;
extern f32 lbl_80796ABC;
}

/* the mangled callees: declared at C++ scope so the call reaches the target's mangled name (rule 9) */
void rotMatrixX(u32 angle, nw4r::math::MTX34* mtx);

void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatZ(u32 angle, nw4r::math::MTX34* mtx);

s32 ran_suu(s32 max);

#pragma peephole off

/* The eft024 kind-1 state-1 handler. */
extern "C" void fn_801173AC(_EFT* self)
{
    _EFT24_CHARA* work = (_EFT24_CHARA*)self->work_0x38;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);

    work->spin_x = (s16)((f32)work->spin_x * lbl_80796A88);
    work->spin_y = (s16)((f32)work->spin_y * lbl_80796A8C);
    work->spin_z = (s16)((f32)work->spin_z * lbl_80796A90);
    work->spin_x = (s16)(work->spin_x + (s16)((((u16)ran_suu(1) & 0x1FF) - 0x1FF) >> 2));
    work->spin_y = (s16)(work->spin_y + (s16)((((u16)ran_suu(1) & 0xFF) - 0xFF) >> 2));
    work->spin_z = (s16)(work->spin_z + (s16)((((u16)ran_suu(1) & 0xFF) - 0xFF) >> 2));
    work->rot_x = (u16)(work->rot_x + (u16)work->spin_x);
    work->rot_y = (u16)(work->rot_y + (u16)work->spin_y);
    work->rot_z = (u16)(work->rot_z + (u16)work->spin_z);

    mtx34_identity(&mtx);
    rotMatrixX(work->rot_x, &mtx);
    rotLocalMatY(work->rot_y, &mtx);
    rotLocalMatZ(work->rot_z, &mtx);

    work->pos.x += lbl_80796A94 * mtx.m[1][0];
    work->pos.y += lbl_80796A94 * mtx.m[1][1];
    work->pos.z += lbl_80796A94 * mtx.m[1][2];
    work->pos.x *= lbl_80796A98;
    work->pos.y *= lbl_80796A98;
    work->pos.z *= lbl_80796A9C;

    copyVec3(&pos, &work->pos);
    mulVecMat(&pos, &mtx);
    addVec3To(&work->chara[0]->pos_0x04, &pos);
    work->chara[0]->field_0x28 = work->rot_x;
    work->chara[0]->field_0x2C = work->rot_y;
    work->chara[0]->field_0x30 = work->rot_z;

    if (work->chara[0]->pos_0x04.y < lbl_80796AA0) {
        self->state_0x05++;
    } else {
        s32 i;
        for (i = 0; i < work->count; i++) {
            work->chara[i]->move(0);
        }
        eft_res_models_spawn(self, (void**)&work->chara[0], 2, work->count, 0);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * the eft025 player family (0x80117688..0x80117DA4)
 * ------------------------------------------------------------------------------------------------- */

/* The two spawn setters: they gate on the caller record's area and hand the family allocator the
 * kind, then remember the caller as the record's source. */
extern "C" void fn_80117688(_PLW* plw, u8 kind)
{
    u8 area = plw->area_0x16;
    _EFT* rec;

    if (area != get_now_areano()) {
        return;
    }
    rec = fn_80117760(kind, area);
    if (rec != 0) {
        rec->source_0x30 = plw;
    }
}

extern "C" void fn_801176F0(_PLW* plw, f32 value)
{
    u8 area = plw->effect_key_0x1A4;
    _EFT* rec;

    if (area != get_now_areano()) {
        return;
    }
    rec = fn_80117760(3, area);
    if (rec != 0) {
        rec->source_0x30 = plw;
        ((_EFT25_WORK*)rec->work_0x38)->scale = value;
    }
}

/* The family allocator: one 0x10-byte model slot, the family tag 25, one live handle, and the two
 * hooks that travel with the record. */
extern "C" _EFT* fn_80117760(u8 kind, u8 area)
{
    _EFT* rec;

    if (area != get_now_areano()) {
        return 0;
    }
    rec = (_EFT*)(void*)eft_res_slot_get(0x10);
    if (rec == 0) {
        return 0;
    }
    ((_EFT25_WORK*)rec->work_0x38)->count = 1;
    rec->field_0x03 = 25;
    rec->type_0x02 = kind;
    rec->area_0x44 = area;
    eft_state_flags_set(rec, 1, 0);
    rec->release_0x40 = fn_8011781C;
    rec->dispatch_0x34 = fn_80117858;
    return rec;
}

/* The family release: hands the pooled handles back and clears the count. */
extern "C" void fn_8011781C(_EFT* self)
{
    _EFT25_WORK* work = (_EFT25_WORK*)self->work_0x38;

    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->models[0], work->count);
    work->count = 0;
}

/* The state dispatcher: state 0 creates and places, 1 advances, 2 steps the state and 3 destroys. */
extern "C" void fn_80117858(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_80117894(self);
    case 1:
        return fn_80117A1C(self);
    case 2:
        return fn_80117D94(self);
    case 3:
        return fn_80117DA4(self);
    }
}

/* State 0: pool the effect, then seat the record at the source's placement joint (kind 1) or latch
 * the source's motion number (kind 2), and hand the state machine on to state 1. */
extern "C" void fn_80117894(_EFT* self)
{
    _EFT25_WORK* work = (_EFT25_WORK*)self->work_0x38;
    nw4r::math::VEC3 vec;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    self->state_0x05++;
    work->models[0] = res_eft_create(lbl_80791938[self->type_0x02], 0x16, 0);
    if (work->models[0] == 0) {
        fn_80117DA4(self);
        return;
    }
    self->flag_0x01 = 1;
    switch (self->type_0x02) {
    case 1: {
        _PLW* actor = (_PLW*)self->source_0x30;
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        fn_800E0A14(&((_EFT25_PHYSICS*)actor->physics_0x13C)->chr_0x04, 0xb, &mtx);
        setVector3(&vec, lbl_80796AB0, lbl_80796AB4, lbl_80796AB8);
        mulVecMat(&vec, &mtx);
        self->pos_0x18.x = mtx.m[0][3] + vec.x;
        self->pos_0x18.y = mtx.m[1][3] + vec.y;
        self->pos_0x18.z = mtx.m[2][3] + vec.z;
        break;
    }
    case 2: {
        _PLW* actor = (_PLW*)self->source_0x30;
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        work->motion = (u16)Get_motion_no(actor);
        break;
    }
    }
    fn_80117A1C(self);
}

/* Seats the pooled handles each frame from the source's joint matrix or position by type, moves the live ones and
 * advances to state 2 once they all die. */
extern "C" void fn_80117A1C(_EFT* self)
{
    _EFT25_WORK* work = (_EFT25_WORK*)self->work_0x38;
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 vel;
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    s32 i;

    VEC3_ctor(&vec);
    VEC3_ctor(&vel);
    MTX34_ctor(&mtxA);
    MTX34_ctor(&mtxB);

    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (event_demo_ck() == 1) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    switch (self->type_0x02) {
    case 0:
        fn_800E0A14(&((_EFT25_PHYSICS*)((_PLW*)self->source_0x30)->physics_0x13C)->chr_0x04, 0xb, &mtxA);
        setVector3(&vec, lbl_80796AB0, lbl_80796AB4, lbl_80796AB8);
        mulVecMat(&vec, &mtxA);
        self->pos_0x18.x = mtxA.m[0][3] + vec.x;
        self->pos_0x18.y = mtxA.m[1][3] + vec.y;
        self->pos_0x18.z = mtxA.m[2][3] + vec.z;
        /* fall through */
    case 1:
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans((nw4r::ef::Effect*)work->models[i], &self->pos_0x18);
        }
        break;
    case 2: {
        _EFT25_ACTOR* actor = (_EFT25_ACTOR*)self->source_0x30;
        if (actor->field_0x00A != 0xa || actor->motion_0x00C <= 2 ||
            work->motion != (u16)Get_motion_no((_PLW*)actor)) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        self->field_0x10 = actor->angle_0x58 + 0x4000;
        fn_800E0A14(&actor->physics_0x13C->chr_0x04, 0xb, &mtxA);
        setVector3(&vec, lbl_80796AB0, lbl_80796AB4, lbl_80796AB8);
        mulVecMat(&vec, &mtxA);
        mtx34_identity(&mtxB);
        rotLocalMatY(self->field_0x10, &mtxB);
        mtxB.m[0][3] = mtxA.m[0][3] + vec.x;
        self->pos_0x18.x = mtxA.m[0][3] + vec.x;
        mtxB.m[1][3] = mtxA.m[1][3] + vec.y;
        self->pos_0x18.y = mtxA.m[1][3] + vec.y;
        mtxB.m[2][3] = mtxA.m[2][3] + vec.z;
        self->pos_0x18.z = mtxA.m[2][3] + vec.z;
        for (i = 0; i < work->count; i++) {
            ((nw4r::ef::Effect*)work->models[i])->SetRootMtx(mtxB);
        }
        break;
    }
    case 3:
        fn_800E0A14(&((_EFT25_CHARA_SRC*)self->source_0x30)->chr_0x08, 0x10, &mtxA);
        setVector3(&vec, lbl_80796AB4, lbl_80796AB8, lbl_80796ABC);
        mulVecMat(&vec, &mtxA);
        self->pos_0x18.x = mtxA.m[0][3] + vec.x;
        self->pos_0x18.y = mtxA.m[1][3] + vec.y;
        self->pos_0x18.z = mtxA.m[2][3] + vec.z;
        change_paramscale_eff((nw4r::ef::Effect*)work->models[0], work->scale);
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans((nw4r::ef::Effect*)work->models[i], &self->pos_0x18);
        }
        break;
    }

    for (i = 0; i < work->count; i++) {
        if (effect_move((nw4r::ef::Effect*)work->models[i]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    eft_res_models_spawn(self, (void**)&work->models[0], 1, work->count, 0);
}

/* State 2 and state 3: advance the state, then destroy the record. */
extern "C" void fn_80117D94(_EFT* self)
{
    self->state_0x05++;
}

extern "C" void fn_80117DA4(_EFT* self)
{
    eft_res_slot_release(self);
}
