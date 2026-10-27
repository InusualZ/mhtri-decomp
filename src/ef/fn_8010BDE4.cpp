/* ef/fn_8010BDE4.cpp - the frame family's per-frame handlers: `fn_8010BDE4` seeds two 0x48-byte per-model records and
 *   hands off to `fn_8010C0E0`, which places them along their angles; `fn_8010C454` and `fn_8010C464` are the state
 *   bump and the slot release `ef/eft013_fx.cpp`'s `fn_8010BDA8` dispatcher routes to.
 * RANGE. .text 0x8010BDE4-0x8010C468 (4 functions); extab 0x8000C1F4-0x8000C204, extabindex 0x800261D8-0x800261F0,
 *   .sdata2 0x807967E8-0x80796810 (0.5, 1.0, 14.0, 0.0, -1.0, 2.0, 0.8, the int-to-float magic).
 *   Seam: the light family from 0x8010C468 (`ef/eft016.cpp`) pools 0.0, 1.0, 0.5, -1.0 and the magic again, so it is
 *   another object; `eft015_set` (0x8010BBE0) installs `fn_8010BDA8`, so the family's setter, release and dispatcher
 *   (0x8010BBE0-0x8010BDE4, registered in `ef/eft013_fx.cpp`) are a candidate for this unit (request filed).
 * NAMES. The map has only `fn_` stems for this range, so the definitions are `extern "C"`; the unit is C++ because
 *   every callee it reaches is a C++ mangling (`MHchar` members, nw4r functions).
 * FLAGS. `cflags_main`; `#pragma peephole off` over the file (retail keeps the unfused `clrlwi`/`cmpwi` pairs).
 * RESIDUALS.
 *  - `fn_8010C0E0`: ours keeps the work pointer where retail reloads `+0x30`'s byte (`lbz r0,0(r3)`), and the
 *    registers are coloured one lower;
 *  - `fn_8010BDE4`: retail keeps the three step bases 0x300/0x900/0x400 in `r30`/`r31`/`r21` (a 0x10 larger frame,
 *    `_savegpr_21`) where ours folds them into the `addi`s (`_savegpr_24`).
 *   flipcheck: `.text` differs (the two rows above); `.sdata2` is byte-identical.
 * SHAPES. The work block is a view of `_EFT::work_0x38`.  The pool is written as literals in retail's creation order:
 *   the water scale's 0.5/1.0, the model scale 1.0, the vector 14/0/0, then -1.0, 2.0 and 0.8 inline at their uses
 *   (hoisting them into locals pools -1.0 before 14.0).
 */

#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "enemy/em_act_leap_ck.h" /* fn_8012E2D4 (rule 2: the owner's header) */
#include "enemy/em_status_ck.h" /* em_status_ck (rule 2: the owner's header) */
#include "enemy/fn_80137614.h" /* fn_80137614 (rule 2: the owner's header) */
#include "enemy/fn_80137648.h" /* fn_80137648 (rule 2: the owner's header) */
#include "types.h"

#pragma peephole off
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/pRoot.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(void*, u8, u8))eft_state_flags_set)

/* ---------------------------------------------------------------------------------------------------
 * the two per-family `_EFT::work_0x38` blocks
 * ------------------------------------------------------------------------------------------------- */

/* One per-model record of the frame family: the model the frame is placed on, its three vectors, the
 * two rotation accumulators and their per-frame steps. size: 0x48 */
struct _EFT_FRAME_ENTRY {
    /* +0x00 */ u8 state_0x00;     /* 0 spawn, 1/2 the two placement shapes */
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ VEC3 vecA_0x08;    /* scaled by the block's +0x98 each frame */
    /* +0x14 */ VEC3 vecB_0x14;    /* rotated by the effect's y angle */
    /* +0x20 */ VEC3 vecC_0x20;    /* the composed placement */
    /* +0x2c */ u32 angle_0x2c;    /* the x/z rotation accumulator */
    /* +0x30 */ u32 angle_0x30;    /* the y rotation accumulator */
    /* +0x34 */ u32 pad_0x34;
    /* +0x38 */ u32 step_0x38;     /* per-frame step of +0x2c */
    /* +0x3c */ u32 step_0x3c;     /* per-frame step of +0x30 */
    /* +0x40 */ u32 step_0x40;     /* seeded but unused by the two handlers below */
    /* +0x44 */ s32 timer_0x44;    /* the per-model frame countdown */
}; /* size: 0x48 */

/* The frame family's block: a count, two fixed records and the two shared per-frame scales.
 * size: 0x9C */
struct _EFT_FRAME_WORK {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ _EFT_FRAME_ENTRY entries[2];
    /* +0x94 */ f32 scale_a_0x94;  /* the per-frame y drop */
    /* +0x98 */ f32 scale_b_0x98;  /* the per-frame vector scale */
}; /* size: 0x9C */

/* ---------------------------------------------------------------------------------------------------
 * the callees (real C++ declarations - rule 9: never the mangled spelling)
 * ------------------------------------------------------------------------------------------------- */

s32 ran_suu(s32 range);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatZ(u32 angle, nw4r::math::MTX34* mtx);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* The unmangled `fn_XXXXXXXX` callees: inside a linkage block they need no per-declaration spelling. */
extern "C" {
void vec3_scale_in_place(nw4r::math::VEC3* v, f32 s);
void addVec3To(nw4r::math::VEC3* out, nw4r::math::VEC3* in);
void eft_res_slot_release(void* self);
u32 eft_res_models_spawn(void* self, void* list, s32 a, s32 b, void* c);
u32 eft_water_state_ck(void* self);
void mtx34_set_trans(nw4r::math::MTX34* m, nw4r::math::VEC3* v);
}

/* The unit's own functions, forward-declared (extern "C" keeps the map's plain spelling). */
extern "C" {
void fn_8010BDE4(_EFT* self);
void fn_8010C0E0(_EFT* self);
void fn_8010C454(_EFT* self);
void fn_8010C464(_EFT* self);
}

/* ---------------------------------------------------------------------------------------------------
 * the frame family
 * ------------------------------------------------------------------------------------------------- */

/* Seeds the two per-model frame records and hands off to the placement handler. */
extern "C" void fn_8010BDE4(_EFT* self)
{
    _EFT_FRAME_WORK* work = (_EFT_FRAME_WORK*)self->work_0x38;
    nw4r::math::MTX34 mtx;
    f32 scale;
    s32 base_a;
    s32 base_b;
    s32 base_c;
    s32 i;

    MTX34_ctor(&mtx);

    self->state_0x05++;
    scale = (eft_water_state_ck(self) == 1) ? 0.5f : 1.0f;
    base_a = 0x300;
    base_b = 0x900;
    base_c = 0x400;

    for (i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create(work->entries[i].model_0x04, 0x1A, 4) == NULL) {
            fn_8010C464(self);
            return;
        }
        setVector3(&work->entries[i].model_0x04->scale_0x1C, 1.0f, 1.0f, 1.0f);
        setVector3(&work->entries[i].vecB_0x14, 14.0f, 0.0f, 0.0f);
        if (self->type_0x02 == 0) {
            u16 r = (u16)ran_suu(0);
            work->entries[i].timer_0x44 = (r & 3);
            work->entries[i].timer_0x44 = work->entries[i].timer_0x44 + 1;
        } else {
            work->entries[i].timer_0x44 = 0;
        }
        if (i == 1) {
            work->entries[i].vecB_0x14.x = -1.0f * work->entries[i].vecB_0x14.x;
            work->entries[i].vecB_0x14.x = work->entries[i].vecB_0x14.x - (f32)((u16)ran_suu(0) % 8);
        } else {
            work->entries[i].vecB_0x14.x = work->entries[i].vecB_0x14.x + (f32)((u16)ran_suu(0) % 8);
        }
        work->entries[i].vecB_0x14.y = (f32)((u16)ran_suu(0) % 15);
        work->entries[i].step_0x38 = base_a + ((u16)ran_suu(0) % 512);
        work->entries[i].step_0x3c = base_b + ((u16)ran_suu(0) % 512);
        work->entries[i].step_0x40 = base_c + ((u16)ran_suu(0) % 512);
        vec3_scale_in_place(&work->entries[i].vecB_0x14, scale);
        work->scale_a_0x94 = 2.0f * scale;
        work->scale_b_0x98 = 0.8f * scale;
    }

    fn_8010C0E0(self);
}

/* Places each frame record's model along its angles for the record's state. */
extern "C" void fn_8010C0E0(_EFT* self)
{
    _EFT_FRAME_WORK* work = (_EFT_FRAME_WORK*)self->work_0x38;
    nw4r::math::MTX34 mtx;
    VEC3 vec;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&vec);

    if (work->entries[0].state_0x00 == 3 && work->entries[1].state_0x00 == 3) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }

    for (i = 0; i < work->count_0x00; i++) {
        work->entries[i].timer_0x44--;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        if (i == 1) {
            rotLocalMatZ(0x8000, &mtx);
        }
        switch (work->entries[i].state_0x00) {
        case 0:
            if (self->source_0x30 == NULL) {
                work->entries[i].timer_0x44 = 0;
            } else if (*(u8*)self->source_0x30 != 0) {
                work->entries[i].timer_0x44 = 0;
            }
            if (work->entries[i].timer_0x44 <= 0) {
                work->entries[i].timer_0x44 = 5;
                work->entries[i].state_0x00++;
                rotVecY(&work->entries[i].vecB_0x14, self->rot_0x24.y);
                rotLocalMatY(work->entries[i].angle_0x30, &mtx);
                addVec3To(&work->entries[i].vecC_0x20, &work->entries[i].vecB_0x14);
                mtx34_set_trans(&mtx, &work->entries[i].vecC_0x20);
                work->entries[i].model_0x04->move2(&mtx, 0);
                eft_res_models_spawn(self, &work->entries[i].model_0x04, 2, 1, NULL);
            } else {
                mtx34_set_trans(&mtx, &work->entries[i].vecC_0x20);
                work->entries[i].model_0x04->move2(&mtx, 0);
                addVec3To(&work->entries[i].vecC_0x20, &work->entries[i].vecA_0x08);
                eft_res_models_spawn(self, &work->entries[i].model_0x04, 2, 1, NULL);
            }
            break;
        case 1:
            rotLocalMatY(work->entries[i].angle_0x30, &mtx);
            mtx.m[0][3] = work->entries[i].vecC_0x20.x;
            mtx.m[1][3] = work->entries[i].vecC_0x20.y;
            mtx.m[2][3] = work->entries[i].vecC_0x20.z;
            if (work->entries[i].timer_0x44 <= 0) {
                work->entries[i].timer_0x44 = 0xF;
                work->entries[i].state_0x00++;
                work->entries[i].model_0x04->move2(&mtx, 0);
                work->entries[i].vecB_0x14.y = work->entries[i].vecB_0x14.y - work->scale_a_0x94;
                vec3_scale_in_place(&work->entries[i].vecA_0x08, work->scale_b_0x98);
                addVec3To(&work->entries[i].vecC_0x20, &work->entries[i].vecB_0x14);
                addVec3To(&work->entries[i].vecC_0x20, &work->entries[i].vecA_0x08);
                work->entries[i].angle_0x30 = work->entries[i].angle_0x30 + work->entries[i].step_0x3c;
                eft_res_models_spawn(self, &work->entries[i].model_0x04, 2, 1, NULL);
            }
            break;
        case 2:
            rotLocalMatZ(work->entries[i].angle_0x2c, &mtx);
            rotLocalMatX(work->entries[i].angle_0x2c, &mtx);
            rotLocalMatY(work->entries[i].angle_0x30, &mtx);
            mtx.m[0][3] = work->entries[i].vecC_0x20.x;
            mtx.m[1][3] = work->entries[i].vecC_0x20.y;
            mtx.m[2][3] = work->entries[i].vecC_0x20.z;
            if (work->entries[i].timer_0x44 <= 0) {
                work->entries[i].state_0x00++;
                work->entries[i].model_0x04->move2(&mtx, 0);
                work->entries[i].vecB_0x14.y = work->entries[i].vecB_0x14.y - work->scale_a_0x94;
                vec3_scale_in_place(&work->entries[i].vecA_0x08, work->scale_b_0x98);
                addVec3To(&work->entries[i].vecC_0x20, &work->entries[i].vecB_0x14);
                addVec3To(&work->entries[i].vecC_0x20, &work->entries[i].vecA_0x08);
                work->entries[i].angle_0x30 = work->entries[i].angle_0x30 + work->entries[i].step_0x3c;
                work->entries[i].angle_0x2c = work->entries[i].angle_0x2c + work->entries[i].step_0x38;
                eft_res_models_spawn(self, &work->entries[i].model_0x04, 2, 1, NULL);
            }
            break;
        }
    }
}

/* The player-side state hook the next unit's dispatcher tail-calls. */
extern "C" void fn_8010C454(_EFT* self)
{
    self->state_0x05++;
}

/* The delete thunk the light family installs as a pool destructor. */
extern "C" void fn_8010C464(_EFT* self)
{
    eft_res_slot_release(self);
}
