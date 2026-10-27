/* ef/eft016.cpp - the player/enemy light effects: `fn_8010C468` pools the effect and its light handles, stores the
 *   family tag 0x10, installs the `fn_8010C77C` dispatcher and the `fn_8010C710` release; three spawn wrappers, two
 *   per-frame states and their light/matrix helpers, and the day-cycle scale helper `fn_8010D1A8` with its two state
 *   thunks.
 * RANGE. .text 0x8010C468-0x8010D2B0 (15 functions); extab 0x8000C204-0x8000C264, extabindex 0x800261F0-0x80026280,
 *   .data 0x8059F518-0x8059F588, .sdata 0x80791820-0x80791828, .sdata2 0x80796810-0x80796848.
 *   Seam: the frame family before 0x8010C468 (`ef/fn_8010BDE4.cpp`) pools 0.0, 1.0, 0.5, -1.0 and the int-to-float
 *   magic too, which one object pools once; `fn_8010D1A8`'s two `.data` tables sit between `fn_8010CFCC`'s
 *   (0x8059F518, 0x8059F574), and only this unit calls `fn_8010D1A8`, `fn_8010D29C` and `fn_8010D2AC`.
 * NAMES. GUESS: the file name - the ef units are named by the family tag their allocator stores at `_EFT+0x03`, and
 *   `fn_8010C468` stores 0x10; the data `eft_slot_offset_scales`, `eft_slot_angle_scales`, `eft_day_scale_keys`,
 *   `eft_day_key_offsets` and `eft_light_model_res_ids`, from their readers.  The map has only `fn_` stems for most
 *   of this range, so the definitions are `extern "C"`; the unit is C++ because every callee it reaches is a C++
 *   mangling (`MHchar` members, nw4r functions).
 * FLAGS. `cflags_main`; `#pragma peephole off` over the file (retail keeps the unfused `clrlwi`/`cmpwi` pairs);
 *   `#pragma fp_contract off` around `fn_8010D1A8` (retail keeps `fmuls` + `fadds` for the interpolation).
 * RESIDUALS.
 *  - `fn_8010C8F8`, `fn_8010CE80`: ours save the registers through `_savegpr_27` and a frame 0x10 larger where retail
 *    stores four registers inline, and invert the null/flag test before the state bump.
 *   The other partial rows (`fn_8010C7B8`, `fn_8010CD68`, `fn_8010C710`) have no recorded cause
 *   (`symdiff.py -u ef/eft016 --all`).
 *   flipcheck: `.text` differs (the rows above); `.data`, `.sdata` and `.sdata2` are emitted and byte-identical.
 * SHAPES. The light family's work block is a view of `_EFT::work_0x38`.  The pool is written as literals: the angle
 *   in `fn_8010CFCC` is one expression passed to `rotVecZ` (0.5 is pooled before 65536/360), `rot.y += 2048.0f` keeps
 *   retail's `fadds` operand order, and `copyVec3` takes `setVec3`'s result.  `fn_8010D1A8` indexes the segment
 *   through its own pointer, computes the slope first and scales the result with `*=` (retail's `fmuls` into `f0`
 *   + `fmr`).
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
#include "ef/eft_res.h" /* eft_res_slot_get, eft_res_slot_release, eft_res_model_get, eft_res_models_release,
                         * eft_res_models_spawn, res_eft_model_create (rule 2) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(void*, u8, u8))eft_state_flags_set)

/* ---------------------------------------------------------------------------------------------------
 * the two per-family `_EFT::work_0x38` blocks
 * ------------------------------------------------------------------------------------------------- */

/* The light family's block, allocated by `eft_res_slot_get(0x50)`: the pooled light handles the state
 * machine walks, the effect id the matrix/scale getters are keyed on, the offset vector and the mode
 * byte that selects the source family. size: 0x50 */
struct _EFT_LIGHT_WORK {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ s32 count_0x18;          /* number of handles at +0x1c */
    /* +0x1c */ MHchar* models_0x1c[5];  /* the pooled light/model handles */
    /* +0x30 */ u8 count2_0x30;          /* the per-frame model count the placement loop runs */
    /* +0x31 */ u8 pad_0x31[3];
    /* +0x34 */ u32 id_0x34;             /* the joint/parameter id the getters take */
    /* +0x38 */ VEC3 vec_0x38;           /* the per-model offset the placement adds */
    /* +0x44 */ f32 scale_0x44;          /* the parameter scale */
    /* +0x48 */ u8 mode_0x48;            /* source family: 0 player, 1 enemy status, 2 enemy action */
    /* +0x49 */ u8 field_0x49;           /* the action/part id passed to the checkers */
    /* +0x4a */ u16 field_0x4a;          /* its second half */
    /* +0x4c */ s16 timer_0x4c;          /* the area-colour re-check countdown */
}; /* size: 0x50 */

/* The `_PLW` bytes the light family reads that `pl.h` does not name yet (a view; `pl.h` owns the
 * type). size: 0x18 */
struct _EFT_PLW_VIEW {
    /* +0x00 */ u8 slot_active;
    /* +0x01 */ u8 pad_0x01[6];
    /* +0x07 */ u8 field_0x07;  /* the per-frame model count copied into the work block */
    /* +0x08 */ u8 pad_0x08[0x0E];
    /* +0x16 */ u8 area_0x16;
}; /* size: 0x18 */

/* The `_ENEMY_WORK` bytes the light family reads that `enemy/ENEMY_WORK.h` does not name yet (a view;
 * that header owns the type). size: 0x952 */
struct _EFT_ENEMY_VIEW {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 pad_0x001[0x1A3];
    /* +0x1A4 */ u8 field_0x1a4;  /* the area byte the action family copies onto the effect */
    /* +0x1A5 */ u8 pad_0x1a5[0x3C];
    /* +0x1E1 */ u8 field_0x1e1;  /* the area byte the status family copies onto the effect */
    /* +0x1E2 */ u8 pad_0x1e2[3];
    /* +0x1E5 */ u8 field_0x1e5;  /* the state/action pair the status gate reads */
    /* +0x1E6 */ u8 field_0x1e6;
    /* +0x1E7 */ u8 pad_0x1E7[0x4B];
    /* +0x232 */ s16 field_0x232; /* the action family's hit countdown */
    /* +0x234 */ u8 pad_0x234[0x6E2];
    /* +0x916 */ s16 field_0x916; /* the status family's hit countdown */
    /* +0x918 */ u8 pad_0x918[0x38];
    /* +0x950 */ s16 field_0x950; /* the status family's second countdown */
}; /* size: 0x952 */

/* The object `sound_job_request` hands back, whose owner slot this unit writes. size: 0x30 - lower bound */
struct _EFT_LIGHT_OBJ {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ _EFT* owner_0x2C;
}; /* size: 0x30 - lower bound */

/* The `MHchar` model bytes this unit reads past what `pl.h` names. size: 0x11C - lower bound */
struct _EFT_MODEL_VIEW {
    /* +0x000 */ u8 pad_0x000[0x10C];
    /* +0x10C */ void* slot_0x10C;
    /* +0x110 */ u8 pad_0x110[4];
    /* +0x114 */ s32 field_0x114;
    /* +0x118 */ s32 field_0x118;
}; /* size: 0x11C - lower bound */

/* ---------------------------------------------------------------------------------------------------
 * the callees (real C++ declarations - rule 9: never the mangled spelling)
 * ------------------------------------------------------------------------------------------------- */

u32 Pl_act_ck(_PLW* plw, u8 a, u16 b);
s32 em_work_die_ck(_ENEMY_WORK* enemy);
u32 get_now_areano();
nw4r::math::MTX34 get_current_view_mtx();
f32 get_em_scale(_ENEMY_WORK* enemy);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* mtx);
void rotVecY(nw4r::math::VEC3* v, u32 angle);
void scaleMat34W(nw4r::math::MTX34* mtx, nw4r::math::VEC3* v);

/* The unmangled `fn_XXXXXXXX` callees: inside a linkage block they need no per-declaration spelling. */
extern "C" {
void mtx34_identity(nw4r::math::MTX34* mtx);
void mtx34_copy(nw4r::math::MTX34* dst, nw4r::math::MTX34* src);
void mtx34_copy_ps(nw4r::math::MTX34* dst, nw4r::math::MTX34* src);
void mtx34_concat(nw4r::math::MTX34* a, nw4r::math::MTX34* b, nw4r::math::MTX34* m);
void g3d_root_model_bind(s32 root, u32 id);
void mtx34_inverse(nw4r::math::MTX34* out, nw4r::math::MTX34* src);
s32 res_node_mtx_id_get(void* handle);
void fn_800DAA4C(nw4r::math::VEC3* pos);
s32 mhchar_wld_mtx_array_get(MHchar* model);
void mhchar_draw_mode_set(MHchar* model, s32 value);
void* sound_job_request(s32 a, u8 b, s32 c, s32 d, s32 e, void (*cb)(void*));
void pl_joint_mtx_get(_PLW* plw, s32 id, nw4r::math::MTX34* mtx);
u32 ai_npc_motion_step_ck(_ENEMY_WORK* enemy, u8 a, u16 b);
void ai_npc_joint_mtx_get(_ENEMY_WORK* enemy, u32 id, nw4r::math::MTX34* mtx);
}

/* The unit's own functions, forward-declared (extern "C" keeps the map's plain spelling). */
extern "C" {
_EFT* fn_8010C468(u32 count);
void eft_pl_hit_light_spawn(_PLW* source);
void eft_stun_set(void* source, u32 id, nw4r::math::VEC3* vec, f32 scale, u8 type);
void eft_npc_aura_create(void* source);
void fn_8010C710(_EFT* self);
void fn_8010C77C(_EFT* self);
void fn_8010C7B8(_EFT* self);
void fn_8010C8F8(_EFT* self);
void fn_8010CD68(_EFT* self, MHchar** models, s32 count);
void fn_8010CE80(void* arg);
void fn_8010CFCC(_EFT* self, nw4r::math::MTX34* out, nw4r::math::MTX34* mtx, s16 index);
void fn_8010D12C(nw4r::math::MTX34* a, nw4r::math::MTX34* b);
f32 fn_8010D1A8(_EFT* self, s32 arg1, f32 farg0);
void fn_8010D29C(_EFT* self);
void fn_8010D2AC(_EFT* self);
}

/* A day-cycle keyframe: the position the segment starts at and the scale reached there. size: 0x08 */
struct EftDayKey {
    /* +0x00 */ f32 pos;
    /* +0x04 */ f32 scale;
}; /* size: 0x08 */

extern "C" {
/* The per-slot forward offsets `fn_8010CFCC` scales by the work block's scale. */
f32 eft_slot_offset_scales[5] = {31.0f, 19.0f, 27.0f, 17.0f, 22.0f};
/* The day-cycle scale keyframes `fn_8010D1A8` interpolates, ended by a -1 position. */
EftDayKey eft_day_scale_keys[6] = {
    {0.0f, 1.3f}, {12.0f, 0.2f}, {15.0f, 0.2f}, {24.0f, 1.3f}, {32.0f, 1.3f}, {-1.0f, 0.0f},
};
/* The per-area offset into the day cycle `fn_8010D1A8` adds to the record's timer. */
u32 eft_day_key_offsets[5] = {0, 13, 25, 7, 17};
/* The per-slot angle scales `fn_8010CFCC` reads. */
f32 eft_slot_angle_scales[5] = {0.0f, 30.0f, -20.0f, 20.0f, -30.0f};
/* The light-model resource id `fn_8010C7B8` creates, per effect type. */
u8 eft_light_model_res_ids[5] = {0x24, 0x24, 0x37, 0x37, 0x00};
}

/* ---------------------------------------------------------------------------------------------------
 * the light family
 * ------------------------------------------------------------------------------------------------- */

/* Allocates a light effect with `count` pooled handles, installs its two hooks and clears its pose. */
extern "C" _EFT* fn_8010C468(u32 count)
{
    _EFT* eft = (_EFT*)eft_res_slot_get(0x50);
    _EFT_LIGHT_WORK* work;
    s32 i;

    if (eft == NULL) {
        return NULL;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    work->count_0x18 = (u8)count;
    eft->dispatch_0x34 = fn_8010C77C;
    eft->release_0x40 = fn_8010C710;

    for (i = 0; i < work->count_0x18; i++) {
        work->models_0x1c[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x1c[i] == NULL) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 0x10;
    eft->rot_0x24.z = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.x = 0;
    eft->timer_0x0C = 0;
    eft->flag_0x01 = 1;
    return eft;
}

/* The five-light player hit effect: no offset, a full second of life. */
extern "C" void eft_pl_hit_light_spawn(_PLW* source)
{
    _EFT* eft = fn_8010C468(5);
    _EFT_LIGHT_WORK* work;

    if (eft == NULL) {
        return;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    eft->type_0x02 = 0;
    eft->source_0x30 = source;
    work->mode_0x48 = 0;
    work->id_0x34 = 0xD;
    work->vec_0x38.x = 0.0f;
    work->vec_0x38.y = 0.0f;
    work->vec_0x38.z = 0.0f;
    work->scale_0x44 = 1.0f;
    eft->area_0x44 = source->area_0x16;
    work->count2_0x30 = 5;
    eft_state_flags_set_c1(eft, 1, 0);
}

/* The parameterised one-light player effect. */
extern "C" void eft_stun_set(void* source, u32 id, nw4r::math::VEC3* vec, f32 scale, u8 type)
{
    _EFT* eft = fn_8010C468(1);
    _EFT_LIGHT_WORK* work;

    if (eft == NULL) {
        return;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    eft->type_0x02 = type;
    eft->source_0x30 = source;
    work->mode_0x48 = 1;
    work->id_0x34 = id;
    copyVec3(&work->vec_0x38, vec);
    work->scale_0x44 = scale;
    eft->area_0x44 = ((_EFT_ENEMY_VIEW*)source)->field_0x1e1;
    work->count2_0x30 = 5;
    eft_state_flags_set_c1(eft, 1, 0);
}

/* The five-light player effect with a raised offset. */
extern "C" void eft_npc_aura_create(void* source)
{
    _EFT* eft = fn_8010C468(5);
    _EFT_LIGHT_WORK* work;

    if (eft == NULL) {
        return;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    eft->type_0x02 = 0;
    eft->source_0x30 = source;
    work->mode_0x48 = 2;
    work->id_0x34 = 0xE;
    work->vec_0x38.x = 0.0f;
    work->vec_0x38.y = 10.0f;
    work->vec_0x38.z = 0.0f;
    work->scale_0x44 = 1.0f;
    eft->area_0x44 = ((_EFT_ENEMY_VIEW*)source)->field_0x1a4;
    work->count2_0x30 = 5;
    eft_state_flags_set_c1(eft, 1, 0);
}

/* The release hook: frees the effect's pooled handles and clears the count. */
extern "C" void fn_8010C710(_EFT* self)
{
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    s32 i;

    for (i = 0; i < work->count_0x18; i++) {
        eft_res_models_release(&work->models_0x1c[i], 1);
    }

    work->count_0x18 = 0;
}

/* The state dispatcher: the first two states are this unit's, the last two the next unit's. */
extern "C" void fn_8010C77C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_8010C7B8(self);
        break;
    case 1:
        fn_8010C8F8(self);
        break;
    case 2:
        fn_8010D29C(self);
        break;
    case 3:
        fn_8010D2AC(self);
        break;
    }
}

/* State 0: creates each pooled light's model from the type's id table and seats it on the source. */
extern "C" void fn_8010C7B8(_EFT* self)
{
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    s32 i;

    self->state_0x05++;

    if (self->type_0x02 >= 4) {
        fn_8010D2AC(self);
        return;
    }

    for (i = 0; i < work->count_0x18; i++) {
        MHchar* model = work->models_0x1c[i];
        if (res_eft_model_create(model, eft_light_model_res_ids[self->type_0x02], 0) == NULL) {
            fn_8010D2AC(self);
            return;
        }
        model->field_0x30 = 0;
        model->field_0x2C = 0;
        model->field_0x28 = 0;
        setVector3(&model->scale_0x1C, 1.0f, 1.0f, 1.0f);
    }

    if (self->source_0x30 == NULL) {
        self->state_0x05 = 2;
        return;
    }

    switch (work->mode_0x48) {
    case 0:
        work->field_0x49 = ((_EFT_PLW_VIEW*)self->source_0x30)->field_0x07;
        work->field_0x4a = ((_EFT_PLW_VIEW*)self->source_0x30)->pad_0x08[0];
        break;
    case 2:
        work->field_0x49 = ((_EFT_ENEMY_VIEW*)self->source_0x30)->field_0x1e1;
        work->field_0x4a = ((_EFT_ENEMY_VIEW*)self->source_0x30)->pad_0x1e2[0];
        work->timer_0x4c = 0;
        break;
    }

    fn_8010C8F8(self);
}

/* State 1: the per-frame placement. */
extern "C" void fn_8010C8F8(_EFT* self)
{
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    VEC3 vec;
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    _EFT_PLW_VIEW* plw;
    _EFT_ENEMY_VIEW* em;
    f32 f31;
    s32 i;

    MTX34_ctor(&mtxA);
    MTX34_ctor(&mtxB);
    VEC3_ctor(&vec);

    if (self->source_0x30 == NULL) {
        self->state_0x05++;
        return;
    }

    plw = (_EFT_PLW_VIEW*)self->source_0x30;
    em = (_EFT_ENEMY_VIEW*)self->source_0x30;

    switch (work->mode_0x48) {
    case 0:
        if (plw->slot_active == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        if (Pl_act_ck((_PLW*)self->source_0x30, work->field_0x49, work->field_0x4a) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        pl_joint_mtx_get((_PLW*)self->source_0x30, work->id_0x34, &mtxA);
        f31 = 1.0f;
        work->count2_0x30 = plw->field_0x07;
        self->area_0x44 = plw->area_0x16;
        break;
    case 1:
        if (em_work_die_ck((_ENEMY_WORK*)self->source_0x30) == 1) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        switch (self->type_0x02) {
        case 0:
            if (((em_status_ck((_ENEMY_WORK*)self->source_0x30, 0x10) == 1 &&
                  em_act_leap_ck(em->field_0x1e5, em->field_0x1e6) == 1 && em->field_0x950 > 0) ||
                 fn_80137614((_ENEMY_WORK*)self->source_0x30) == 1 ||
                 fn_80137648((_ENEMY_WORK*)self->source_0x30) == 1)) {
                break;
            }
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        case 1:
            if (em->field_0x916 > 0) {
                break;
            }
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        case 2:
            if (((em_status_ck((_ENEMY_WORK*)self->source_0x30, 0x10) == 1 &&
                  em_act_leap_ck(em->field_0x1e5, em->field_0x1e6) == 1 && em->field_0x950 > 0) ||
                 fn_80137614((_ENEMY_WORK*)self->source_0x30) == 1 ||
                 fn_80137648((_ENEMY_WORK*)self->source_0x30) == 1)) {
                break;
            }
            self->flag_0x01 = 0;
            return;
        case 3:
            if (em->field_0x916 > 0) {
                break;
            }
            self->flag_0x01 = 0;
            return;
        }
        get_joint_wmat_em((_ENEMY_WORK*)self->source_0x30, work->id_0x34, &mtxA);
        f31 = get_em_scale((_ENEMY_WORK*)self->source_0x30);
        self->area_0x44 = em->field_0x1e1;
        break;
    case 2:
        if (em->active != 0 &&
            ai_npc_motion_step_ck((_ENEMY_WORK*)self->source_0x30, work->field_0x49, work->field_0x4a) != 0 &&
            em->field_0x232 > 0) {
            ai_npc_joint_mtx_get((_ENEMY_WORK*)self->source_0x30, work->id_0x34, &mtxA);
            f31 = 1.0f;
            self->area_0x44 = em->field_0x1a4;
            break;
        }
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }

    self->flag_0x01 = 1;
    self->rot_0x24.y += 2048.0f;

    if (self->type_0x02 >= 2) {
        MHchar* model = work->models_0x1c[0];
        mhchar_draw_mode_set(model, 1);
        model->move(0);
        fn_8010CD68(self, work->models_0x1c, work->count_0x18);
    } else {
        for (i = 0; i < work->count2_0x30; i++) {
            MHchar* model = work->models_0x1c[i];
            f32 s;
            fn_8010CFCC(self, &mtxB, &mtxA, i);
            fn_8010D12C(&mtxB, &mtxB);
            s = fn_8010D1A8(self, i, f31);
            setVector3(&model->scale_0x1C, s, s, s);
            mhchar_draw_mode_set(model, 1);
            model->move2(&mtxB, 0);
        }
    }

    eft_res_models_spawn(self, (void**)&work->models_0x1c[0], 2, work->count2_0x30, NULL);
    self->timer_0x0C++;
    if (self->timer_0x0C >= 0x20) {
        self->timer_0x0C = 0;
        if (work->mode_0x48 == 0) {
            work->timer_0x4c--;
            if (work->timer_0x4c <= 0) {
                work->timer_0x4c = 0x2D;
                if (self->area_0x44 == get_now_areano()) {
                    setVector3(&vec, mtxA.m[0][3], mtxA.m[1][3], mtxA.m[2][3]);
                    fn_800DAA4C(&vec);
                }
            }
        }
    }
}

/* Places each pooled model: either the two-model enemy case or the per-model light case. */
extern "C" void fn_8010CD68(_EFT* self, MHchar** models, s32 count)
{
    VEC3 vec;
    s32 i;

    VEC3_ctor(&vec);

    if ((u8)self->area_0x44 != (u8)get_now_areano()) {
        return;
    }

    if (self->flag_0x01 == 0) {
        for (i = 0; i < count; i++) {
            MHchar* model = models[i];
            if (model == NULL) {
                continue;
            }
            if (((_EFT_MODEL_VIEW*)model)->field_0x118 ==
                *(u32*)((u8*)((_EFT_MODEL_VIEW*)model)->slot_0x10C + 4)) {
                g3d_root_model_bind(pRoot, 0);
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            MHchar* model = models[i];
            void* p;
            if (model == NULL) {
                continue;
            }
            if (((_EFT_MODEL_VIEW*)model)->field_0x118 !=
                *(u32*)((u8*)((_EFT_MODEL_VIEW*)model)->slot_0x10C + 4)) {
                continue;
            }
            p = sound_job_request(1, model->ready, 0, 0x40, 0, fn_8010CE80);
            if (p != NULL) {
                ((_EFT_LIGHT_OBJ*)p)->owner_0x2C = self;
                g3d_root_model_bind(pRoot, ((_EFT_MODEL_VIEW*)model)->field_0x118);
            }
        }
    }
}

/* The light-object callback `sound_job_request` installs: places the effect's models on the enemy joint. */
extern "C" void fn_8010CE80(void* arg)
{
    _EFT* self = ((_EFT_LIGHT_OBJ*)arg)->owner_0x2C;
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    _ENEMY_WORK* enemy = (_ENEMY_WORK*)self->source_0x30;
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    f32 f31;
    s16 i;

    MTX34_ctor(&mtxA);
    MTX34_ctor(&mtxB);

    if (enemy != NULL && work->mode_0x48 == 1) {
        if (em_work_die_ck(enemy) == 1) {
            return;
        }
        get_joint_wmat_em(enemy, work->id_0x34, &mtxA);
        f31 = get_em_scale(enemy);
    }

    for (i = 0; i < work->count2_0x30; i++) {
        VEC3 scale;
        f32 s;
        s32 base;
        s32 handle;
        s32 idx;
        fn_8010CFCC(self, &mtxB, &mtxA, i);
        fn_8010D12C(&mtxB, &mtxB);
        s = fn_8010D1A8(self, i, f31);
        setVec3(&scale, s, s, s);
        scaleMat34W(&mtxB, &scale);
        base = mhchar_wld_mtx_array_get((MHchar*)((_EFT_MODEL_VIEW*)work->models_0x1c[i])->field_0x118);
        handle = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(&((_EFT_MODEL_VIEW*)work->models_0x1c[i])->field_0x114)->GetResNode(i + 1).mpData;
        idx = res_node_mtx_id_get(&handle);
        mtx34_copy_ps(&((nw4r::math::MTX34*)base)[idx], &mtxB);
    }
}

/* The per-model matrix builder: rotates the model's offset by its angle and composes the effect
 * matrix into `out`. */
extern "C" void fn_8010CFCC(_EFT* self, nw4r::math::MTX34* out, nw4r::math::MTX34* mtx, s16 index)
{
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    VEC3 v20;
    VEC3 v14;
    VEC3 v8;

    VEC3_ctor(&v20);
    VEC3_ctor(&v14);
    copyVec3(&v14, &work->vec_0x38);
    mulVecMat(&v14, mtx);
    copyVec3(&v20, setVec3(&v8, 0.0f, 0.0f, work->scale_0x44 * eft_slot_offset_scales[index]));
    rotVecY(&v20, self->rot_0x24.y + index * 0x3333);
    rotVecZ(&v20, (u16)(s32)(0.5f + (65536.0f * eft_slot_angle_scales[index]) / 360.0f));
    v20.y += 30.0f;
    mulVecMat(&v20, mtx);
    mtx34_identity(out);
    out->m[0][3] = v20.x + mtx->m[0][3] + v14.x;
    out->m[1][3] = v20.y + mtx->m[1][3] + v14.y;
    out->m[2][3] = v20.z + mtx->m[2][3] + v14.z;
}

/* Applies the current view matrix to the two matrices (the per-model transform). */
extern "C" void fn_8010D12C(nw4r::math::MTX34* a, nw4r::math::MTX34* b)
{
    nw4r::math::MTX34 m;

    MTX34_ctor(&m);
    mtx34_copy(&m, &get_current_view_mtx());
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    mtx34_inverse(&m, &m);
    mtx34_concat(a, b, &m);
}

#pragma fp_contract off
/* Interpolates the effect scale over the day-cycle keyframes and applies the record's own scale. */
extern "C" f32 fn_8010D1A8(_EFT* self, s32 arg1, f32 farg0)
{
    s32 idx;
    _EFT_LIGHT_WORK* work;
    EftDayKey* key;
    s32 pos;
    s32 i;
    f32 slope;
    f32 scale;
    EftDayKey* seg;

    work = (_EFT_LIGHT_WORK*)self->work_0x38;
    key = eft_day_scale_keys;
    pos = self->timer_0x0C + eft_day_key_offsets[(s16)arg1];
    if (pos >= 32) {
        pos -= 32;
    }
    idx = self->timer_0x0C;
    for (i = 1; key[i].pos != -1.0f; i++) {
        if ((f32)pos <= key[i].pos) {
            idx = i - 1;
            break;
        }
    }
    seg = &key[idx];
    slope = (seg[1].scale - seg->scale) / (seg[1].pos - seg->pos);
    scale = work->scale_0x44 * (seg->scale + slope * ((f32)pos - seg->pos));
    scale *= farg0;
    return scale;
}
#pragma fp_contract reset

/* Advances the effect's state (a second copy of `fn_8010C454`). */
extern "C" void fn_8010D29C(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the effect's pool slot (a second copy of `fn_8010C464`). */
extern "C" void fn_8010D2AC(_EFT* self)
{
    eft_res_slot_release(self);
}
