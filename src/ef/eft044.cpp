/*
 * ef/eft044.cpp - the eft044 `_EFT` effect family (`_EFT::field_0x03 = 44`): one model on an enemy's joint that
 *   pops in through a key-framed scale when the enemy's +0x43D status changes, with the pop-in reset and the
 *   joint toggle it calls.
 * RANGE. .text 0x8031E57C-0x8031EB54 (10 functions); extab 0x80016054-0x8001607C, extabindex 0x80034EE4-0x80034F20,
 *   .data 0x805DD248-0x805DD298, .sdata2 0x8079AE98-0x8079AEB0.  One TU: the pool restarts at 0x8079AE98 (1.0f and
 *   the int-to-float double repeat the previous TU's), `menu_eff_reset` alone loads 0x8079AEA8 and its only callers
 *   are `eft044_move`, and the extabindex records 0x80034EE4..0x80034F20 name exactly these framed functions.
 * FLAGS. `cflags_menu` (configure.py, the group of `menu/menu_item_effect.cpp` it was cut from); file-scope
 *   `#pragma pool_data off`.
 * NAMES. The file name is a GUESS from the `field_0x03` family tag (sibling scheme `ef/eft052.cpp`); no `__FILE__`
 *   string covers the range.  GUESS, from the bodies: `eft044_release`, `eft044_dispatch`, `eft044_place`,
 *   `eft044_move`, `eft044_state_step`, `eft044_retire`, `menu_eff_reset`, `menu_eff_model_select`,
 *   `eft_em_model_fx_spawn`, `eft044_scale_keys`.
 * RESIDUALS. none known.
 * SHAPES. `EmAilmentFxBlock::model_fx` is read as an `EmAilmentFx*` (the owner's header types the word `u32`).
 *  - `eft044_move` masks `field_0x07` with `0xFD`/`0xFE` (retail `andi.`/`rlwinm 24,30`; `~2`/`~1` add a `clrlwi`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "fn_8004CAD8/mtx.h"
#include "mh3_pad.h"
#include "ef.h"
#include "pl.h"
#include "Pl/plw.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/EnemyData.h"
#include "enemy/fn_8012BDF4.h"        /* em_work_die_ck */
#include "enemy/fn_8012EC74.h"        /* get_em_chg_scale, get_joint_wmat_em */
#include "enemy/fn_801251D0.h"        /* get_enemy_data */
#include "enemy/em_flag_0x8AD_ck.h"
#include "fn_8004CAD8.h"              /* getKeyData, vec3_scale_in_place, ... */
#include "ef/eft_res.h"
#include "ef/effect.h"                /* eft_state_flags_set */
#include "menu/eft_em_model_fx_spawn.h"

#pragma pool_data off

/* eft044's work block, the `_EFT::work_0x38` record `eft_res_slot_get(8)` pools with the effect. */
struct Eft044Work {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[1];
}; /* size: 0x8 */

extern "C" void eft044_release(_EFT* self);
extern "C" void eft044_dispatch(_EFT* self);
extern "C" void eft044_place(_EFT* self);
extern "C" void eft044_move(_EFT* self);
extern "C" void eft044_state_step(_EFT* self);
extern "C" void eft044_retire(_EFT* self);
extern "C" void menu_eff_reset(_EFT* self, u8 mode);
extern "C" void menu_eff_model_select(_EFT* self, u32 index);

/* eft044's pop-in scale curve, `{frame, scale}` pairs for `getKeyData`, closed by frame -1. */
f32 eft044_scale_keys[] = {
    0.0f, 0.0f,
    2.0f, 0.2f,
    3.0f, 0.4f,
    5.0f, 2.0f,
    9.0f, 0.5f,
    12.0f, 1.2f,
    15.0f, 0.9f,
    18.0f, 1.05f,
    20.0f, 1.0f,
    -1.0f, 1.0f,
};

/* 0x8031E57C (0xEC): pools the eft044 record for `enemy`, installs its two hooks and takes its model. */
extern "C" void eft_em_model_fx_spawn(_ENEMY_WORK* enemy)
{
    _EFT* eft = (_EFT*)eft_res_slot_get(8);
    if (eft == NULL)
        return;

    eft->type_0x02 = 0;
    eft->dispatch_0x34 = eft044_dispatch;
    eft->release_0x40 = eft044_release;
    Eft044Work* work = (Eft044Work*)eft->work_0x38;
    work->count = 1;
    for (s32 i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft_state_flags_set(eft, 1, 0);
    eft->field_0x03 = 44;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
    eft->source_0x30 = enemy;
    eft->flag_0x01 = 1;
}

/* 0x8031E668 (0x3C): hands the pooled models back (the release hook). */
extern "C" void eft044_release(_EFT* self)
{
    Eft044Work* work = (Eft044Work*)self->work_0x38;
    eft_res_models_release(work->models, work->count);
    work->count = 0;
}

/* 0x8031E6A4 (0x3C): dispatches `state_0x05` to its state handler (the dispatch hook). */
extern "C" void eft044_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft044_place(self);
        return;
    case 1:
        eft044_move(self);
        return;
    case 2:
        eft044_state_step(self);
        return;
    case 3:
        eft044_retire(self);
        return;
    }
}

/* 0x8031E6E0 (0x114): creates the model, hides its two extra joints and scales it by the enemy's size. */
extern "C" void eft044_place(_EFT* self)
{
    EmAilmentFx* fx;
    _ENEMY_WORK* enemy = (_ENEMY_WORK*)self->source_0x30;
    Eft044Work* work = (Eft044Work*)self->work_0x38;

    self->state_0x05++;
    if (res_eft_model_create(work->models[0], 57, 24) == NULL) {
        eft044_retire(self);
        return;
    }
    work->models[0]->setVisibility(1, false);
    work->models[0]->setVisibility(2, false);
    fx = (EmAilmentFx*)get_enemy_data(enemy)->ailment_fx_0x9C->model_fx;
    if (fx != NULL) {
        work->models[0]->scale_0x1C.x = fx->scale * get_em_chg_scale(enemy);
        work->models[0]->scale_0x1C.z = fx->scale * get_em_chg_scale(enemy);
    } else {
        work->models[0]->scale_0x1C.x = 1.0f;
        work->models[0]->scale_0x1C.z = 1.0f;
    }
    self->rot_0x24.x = 0;
    self->rot_0x24.y = enemy->field_0x1C0;
    self->rot_0x24.z = 0;
    eft044_move(self);
}

/* 0x8031E7F4 (0x284): restarts the pop-in when the enemy's status flips, then plays the scale curve on the joint. */
extern "C" void eft044_move(_EFT* self)
{
    Eft044Work* work = (Eft044Work*)self->work_0x38;
    _ENEMY_WORK* enemy = (_ENEMY_WORK*)self->source_0x30;
    VEC3 offset;
    nw4r::math::MTX34 mtx;
    VEC3 tmp;

    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);
    if (em_work_die_ck(enemy)) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->area_0x44 = enemy->area_no;
    switch (enemy->field_0x43D) {
    case 0:
        if (self->field_0x07 & 2) {
            self->field_0x06 = 1;
            menu_eff_reset(self, 1);
        }
        self->field_0x07 = (self->field_0x07 | 1) & 0xFD;
        break;
    case 2:
        if (self->field_0x07 & 1) {
            self->field_0x06 = 1;
            menu_eff_reset(self, 0);
        }
        self->field_0x07 = (self->field_0x07 | 2) & 0xFE;
        break;
    default:
        self->field_0x07 &= ~3;
        break;
    }
    if (self->field_0x06 == 0)
        return;
    self->timer_0x0C++;
    if ((f32)self->timer_0x0C > 25.0f) {
        self->field_0x06 = 0;
        return;
    }
    EmAilmentFx* fx = (EmAilmentFx*)get_enemy_data(enemy)->ailment_fx_0x9C->model_fx;
    work->models[0]->scale_0x1C.y = fx->scale * getKeyData(eft044_scale_keys, (f32)self->timer_0x0C);
    self->rot_0x24.y = enemy->field_0x1C0;
    if (fx != NULL) {
        copyVec3(&offset, setVec3(&tmp, fx->x, fx->y, fx->z));
        vec3_scale_in_place(&offset, get_em_chg_scale(enemy));
        get_joint_wmat_em(enemy, fx->joint, &mtx);
        mulVecMat(&offset, &mtx);
        self->pos_0x18.x = mtx.m[0][3] + offset.x;
        self->pos_0x18.y = mtx.m[1][3] + offset.y;
        self->pos_0x18.z = mtx.m[2][3] + offset.z;
    } else {
        self->flag_0x01 = 0;
        self->field_0x06 = 0;
        return;
    }
    copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
    work->models[0]->move(0);
    if (em_flag_0x8AD_ck(enemy) == 0)
        eft_res_models_spawn(self, (void**)work->models, 2, work->count, 0);
}

/* 0x8031EA78 (0x10): advances the effect to its next state. */
extern "C" void eft044_state_step(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8031EA88 (0x4): retires the effect's record. */
extern "C" void eft044_retire(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x8031EA8C (0x50): restarts the pop-in - clears the model's y-scale and the timer, sets the two flag bytes and
 * shows the model `mode` selects. */
extern "C" void menu_eff_reset(_EFT* self, u8 mode) {
    Eft044Work* work = (Eft044Work*)self->work_0x38;

    work->models[0]->scale_0x1C.y = 0.0f;
    self->timer_0x0C = 0;
    self->flag_0x01 = 1;
    self->field_0x06 = 1;

    switch (mode) {
    case 0:
        menu_eff_model_select(self, 1);
        break;
    case 1:
        menu_eff_model_select(self, 2);
        break;
    }
}

/* 0x8031EADC (0x78): shows joint `index` of the model and hides the other of joints 1 and 2. */
extern "C" void menu_eff_model_select(_EFT* self, u32 index) {
    Eft044Work* work = (Eft044Work*)self->work_0x38;
    s32 i;

    for (i = 1; i <= 2; i++) {
        if ((u32)i == index) {
            work->models[0]->setVisibility(i, true);
        } else {
            work->models[0]->setVisibility(i, false);
        }
    }
}
