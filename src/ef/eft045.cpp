/*
 * ef/eft045.cpp - the eft045 `_EFT` effect family (`_EFT::field_0x03 = 0x2D`): three models spawned at a position
 *   and rotation that grow in, with the owner's landing sound when the area is the current one, dispatched on
 *   `state_0x05` with its `+0x38` work record (`MenuFxWork`).
 * RANGE. .text 0x8031EB54-0x8031F304 (7 functions); extab 0x8001607C-0x8001609C, extabindex 0x80034F20-0x80034F50,
 *   .sdata2 0x8079AEB0-0x8079AED8.  One TU: its pool (2.5f, 1.0f, 0.0f, -1.0f, 0.9f, 0.01f, 0.5f, the int-to-float
 *   double) is loaded only by `eft045_start`/`eft045_grow`, the next unit's pool repeats the conversion double, and
 *   the extabindex record 0x80034F50 names 0x8031F304 as the next unit's first function.
 * FLAGS. `cflags_menu` (configure.py, the group of the range it was cut from); file-scope `#pragma pool_data off`,
 *   `#pragma fp_contract off` (retail keeps every multiply and add apart) and `#pragma optimization_level 4`.
 * NAMES. The file name is a GUESS from the `field_0x03` family tag (sibling scheme `ef/eft044.cpp`); no `__FILE__`
 *   string covers the range.  `eft045_set` is the map's own (mangled) name.  GUESS: `eft045_release`,
 *   `eft045_dispatch`, `eft045_start`, `eft045_grow`, `eft045_state_next`, `eft045_retire`, `MenuFxWork`.
 * RESIDUALS. none known.
 */

#include "ef/eft045.h"
#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2: the owner's header) */
#include "types.h"
#include "fn_8004CAD8.h" /* calcVecAngXY / MTX34_ctor / vec3_scale_in_place / userdata_flag_ck (rule 2: its owner) */
#include "ef/eft_res.h" /* eft_res_slot_get / eft_res_models_spawn / res_eft_model_create */
#include "sound/mhchar_draw_mode_set.h"
#include "sound/pl_se_pos_req.h"
#include "lobby/lb_menu_pos_tbl.h" /* getItemListSelection */
#include "stage/stg_w.h" /* get_now_areano */
#include "g3d/g3d_calcmaterial.h" /* res_tex_srt_copy_ctor */
#include "nw4r/g3d/scnmdl.h" /* nw4r::g3d::ScnMdl::CopiedMatAccess */
#include "unsplit/sound.h" /* copied_mat_access_valid_ck */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(_EFT*, u32, u32))eft_state_flags_set)

/* retail addresses each table of this file with its own lis/addi pair, and keeps every float multiply and add
 * apart (fmuls + fadds, no fmadds) */
#pragma pool_data off
#pragma fp_contract off
#pragma optimization_level 4

/* This unit's own forward declarations (defined below). */
extern "C" {
void eft045_release(_EFT* self);
void eft045_dispatch(_EFT* self);
void eft045_start(_EFT* self);
void eft045_grow(_EFT* self);
void eft045_state_next(_EFT* self);
void eft045_retire(_EFT* self);
} /* extern "C" */

/* The mangled callees (real C++ signatures - rule 9). */
struct _CP_VECTOR;
u16 ran_suu(s32 range);
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);
void rotVecZXY(nw4r::math::VEC3* v, _CP_VECTOR* rot);

/* ---------------------------------------------------------------------------------------------- */
/* the effect family wrappers                                                                      */
/* ---------------------------------------------------------------------------------------------- */

/* 0x8031EB54 (0x124): spawns the three-model effect at `pos`/`rot` in area `area` and plays the owner's
 * landing sound when the area is the current one. */
void eft045_set(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, _PLW* plw) {
    _EFT* self = (_EFT*)eft_res_slot_get(0x18);
    MenuFxWork* work;
    s32 i;

    if (self == NULL) {
        return;
    }
    self->dispatch_0x34 = eft045_dispatch;
    self->release_0x40 = eft045_release;
    work = (MenuFxWork*)self->work_0x38;
    work->count = 3;
    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(self);
            return;
        }
    }
    copyVec3(&self->pos_0x18, pos);
    eft_rot_vec_copy(&self->rot_0x24, rot);
    eft_state_flags_set_c1(self, 1, 0);
    self->field_0x03 = 0x2D;
    self->type_0x02 = 0;
    self->area_0x44 = area;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    if (plw != NULL && self->area_0x44 == get_now_areano()) {
        pl_se_pos_req(plw, &self->pos_0x18);
    }
}

/* 0x8031EC78 - release the effect's model list. */
extern "C" void eft045_release(_EFT* self) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    eft_res_models_release(&work->models[0], work->count);
    work->count = 0;
}

/* 0x8031ECB4 - the first family's four-state dispatch on `state_0x05`. */
extern "C" void eft045_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft045_start(self);
        return;
    case 1:
        eft045_grow(self);
        return;
    case 2:
        eft045_state_next(self);
        return;
    case 3:
        eft045_retire(self);
        return;
    }
}

/* 0x8031ECF0 (0x2FC): the first family's state 0 - creates each model at the effect's position, seeds
 * its tilt, scale and spin, and scrolls its texture matrix. */
extern "C" void eft045_start(_EFT* self) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;
    nw4r::math::VEC3 dir;
    s16 i;
    MHchar* model;

    VEC3_ctor(&dir);
    self->state_0x05++;
    work->counter_0x16 = 0;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->models[i], 0x43, 0x10C) == NULL) {
            eft045_retire(self);
            return;
        }
        model = work->models[i];
        copyVec3(&model->pos_0x04, &self->pos_0x18);
        if ((work->counter_0x16 & 1) == 0) {
            eft_rot_vec_copy(&model->rot_0x28, &self->rot_0x24);
            setVector3(&model->scale_0x1C, 2.5f, 2.5f, 1.0f);
        } else {
            setVector3(&dir, 0.0f, 0.0f, 1.0f);
            rotVecZXY(&dir, &self->rot_0x24);
            vec3_scale_in_place(&dir, -1.0f);
            calcVecAngXY(&dir, (u32*)&model->field_0x28, (u32*)&model->field_0x2C);
            model->field_0x30 = 0;
            setVector3(&model->scale_0x1C, 2.5f, 2.5f, 1.0f);
        }
        vec3_scale_in_place(&model->scale_0x1C, 0.9f + 0.01f * (ran_suu(0) % 20));
        model->field_0x30 = ran_suu(0);
        mhchar_draw_mode_set(model, 1);
        model->ready = 1;
        work->value_0x10[i] = i * -6;
        {
            nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)model->field_0x118, 0);
            if (copied_mat_access_valid_ck(&access) != 0) {
                nw4r::g3d::ResTexSrt srt;
                nw4r::math::MTX34 mtx;

                res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
                MTX34_ctor(&mtx);
                srt.GetEffectMtx(0, &mtx);
                mtx.m[0][3] -= 0.5f;
                if (mtx.m[0][3] < 0.0f) {
                    mtx.m[0][3] += 1.0f;
                }
                if (mtx.m[0][3] >= 1.0f) {
                    mtx.m[0][3] -= 1.0f;
                }
                srt.SetEffectMtx(0, &mtx);
            }
        }
        work->counter_0x16++;
    }
    eft045_grow(self);
}

/* 0x8031EFEC (0x304): the first family's state 1 - steps each model's frame counter, re-seeds the
 * models that finished, and cycles the visible joint of the ones still growing. */
extern "C" void eft045_grow(_EFT* self) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;
    nw4r::math::VEC3 dir;
    MHchar* model;
    u32 i;

    VEC3_ctor(&dir);
    for (i = 0; i < work->count; i++) {
        model = work->models[i];
        if (work->value_0x10[i] >= 0) {
            if (work->value_0x10[i] >= 15) {
                if (work->counter_0x16 < 5) {
                    if ((work->counter_0x16 & 1) == 0) {
                        eft_rot_vec_copy(&model->rot_0x28, &self->rot_0x24);
                        setVector3(&model->scale_0x1C, 2.5f, 2.5f, 1.0f);
                    } else {
                        setVector3(&dir, 0.0f, 0.0f, 1.0f);
                        rotVecZXY(&dir, &self->rot_0x24);
                        vec3_scale_in_place(&dir, -1.0f);
                        calcVecAngXY(&dir, (u32*)&model->field_0x28, (u32*)&model->field_0x2C);
                        model->field_0x30 = 0;
                        setVector3(&model->scale_0x1C, 2.5f, 2.5f, 1.0f);
                    }
                    vec3_scale_in_place(&model->scale_0x1C, 0.9f + 0.01f * (ran_suu(0) % 21));
                    model->field_0x30 = ran_suu(0);
                    work->value_0x10[i] = 0;
                    work->counter_0x16++;
                }
            } else {
                u32 n = model->get_joint_num();
                u32 j;
                _GXColor color;

                for (j = 0; j < n; j++) {
                    if (j == work->value_0x10[i]) {
                        model->setVisibility(j, true);
                    } else {
                        model->setVisibility(j, false);
                    }
                }
                model->getMatColor(0, GX_COLOR0A0, &color);
                color.r = 0xD8;
                color.g = 0xE3;
                color.b = 0xFF;
                color.a = 0xFF;
                model->setMatColor(0, GX_COLOR0A0, color, false);
                model->move(0);
                eft_res_models_spawn(self, (void**)&model, 2, 1, 0);
            }
        }
        work->value_0x10[i] += 2;
    }
    if (work->counter_0x16 >= 5) {
        u32 done = 1;

        for (i = 0; i < work->count; i++) {
            if (work->value_0x10[i] < 15) {
                done = 0;
                break;
            }
        }
        if (done == 1) {
            self->state_0x05++;
        }
    }
}

/* 0x8031F2F0 - advance the first family's state. */
extern "C" void eft045_state_next(_EFT* self) {
    self->state_0x05++;
}

/* 0x8031F300 - retire the first family's effect. */
extern "C" void eft045_retire(_EFT* self) {
    eft_res_slot_release(self);
}
