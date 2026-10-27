/*
 * ef/eft043.cpp - the eft043 `_EFT` effect family (`_EFT::field_0x03 = 0x2B`): the player's hand glow on joint 7,
 *   spawned by the crouch acts of `Pl/fn_802840DC.cpp`; the pool (+0x38 `Eft043Work`) holds up to two effects, a
 *   colour and a scale, and `__sinit` builds the two joint offsets.
 * RANGE. .text 0x8031D294-0x8031DAA8 (8 functions); extab 0x80016004-0x80016034, extabindex 0x80034E6C-0x80034EB4,
 *   .ctors 0x8056F398-0x8056F39C, .data 0x805DCED8-0x805DCF38, .bss 0x806BE108-0x806BE120,
 *   .sdata 0x80792C40-0x80792C48, .sdata2 0x8079AE50-0x8079AE70.  One TU, cut from `menu/menu_combo.cpp`: its data
 *   starts on fresh 8-byte boundaries (`.data` 0x805DCED4-0x805DCED8 and `.sdata` 0x80792C3A-0x80792C40 are padding
 *   after the combination screens' tables), every `.sdata2` word, the `.bss` and the `.ctors` word are referenced only
 *   from this range, and the extabindex records 0x80034E6C.. name exactly these functions.
 * FLAGS. `cflags_menu` (configure.py, the group of the unit it was cut from).
 * NAMES. The file name is a GUESS from the `field_0x03` family tag (sibling scheme `ef/eft044.cpp`); no `__FILE__`
 *   string covers the range and the dump answers `zz_`.  GUESS, from the bodies: the `eft043_*` family and the data
 *   names; foreign (registry): `pl_ryugeki_se_req`, `pl_act4_phase_ck`.
 * RESIDUALS. none known.
 */

#include "types.h"
#include "ef/eft_res.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/effect.h"
#include "pl.h" /* _PLW */
#include "gx.h" /* _GXColor */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 */
#include "fn_8004CAD8.h" /* calcVecAngXY / rotVecX / rotVecY / cpSetRotMatrix / getKeyData3 / MTX34_ctor */
#include "sound/fn_800DD1F0.h" /* mhchar_joint_mtx_get */
#include "g3d/g3d_calcworld.h" /* addVec3To */
#include "ef/eft001.h" /* mtx34_set_trans */
#include "ef/mtx34_trans_get.h"
#include "ef/eft_stage_spawn_flag.h"
#include "ef/RetireParticleAll.h"
#include "ef/push_eft_effect_heap_num.h"
#include "unsplit/ef_control.h" /* eft_control */
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_act4_phase_ck.h"
#include "sound/pl_ryugeki_se_req.h"
#include "stage/stg_w.h" /* get_now_areano */
#include "menu/eft043_set.h"

/* The eft043 effect pool.  size: 0x14 */
struct Eft043Work {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ _GXColor color;
    /* +0x10 */ f32 scale;
};

/* The joint holder `_PLW::physics_0x13C` points at: the player's `MHchar` at +0x04 (a view of
 * `ef/fn_80114E34_types.h`'s `_PLW_PHYSICS`, whose header redefines `_EFT`).  size: 0x144 - lower bound */
struct Eft043PlwJoints {
    /* +0x000 */ u8 unused_0x000[0x04];
    /* +0x004 */ MHchar joint_0x004;
};

/* A vector built through `setVec3`, so a file-scope one is constructed by `__sinit`.  size: 0xC */
struct Eft043Offset : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    Eft043Offset(f32 x, f32 y, f32 z) { setVec3(this, x, y, z); }
};

extern "C" {
/* The effect ids type 0 creates, and the pool size of each type. */
u16 eft043_effect_ids[2] = { 0x019F, 0x01A0 };
u8 eft043_effect_counts[2] = { 2, 1 };
/* The colour key strip and the scale key table type 0 runs. */
u8 eft043_color_keys[16] = { 0x00, 0x64, 0x64, 0x64, 0x19, 0x1E, 0x32, 0x96, 0xFF, 0x1E, 0x32, 0x96, 0, 0, 0, 0 };
f32 eft043_scale_keys[20] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    2.0f, 3.2f, 0.3f, 1.5f,
    6.0f, 1.2f, 1.2f, 1.5f,
    25.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, 1.0f,
};
}

/* The joint-7 offset of each type; MWCC's `__sinit` builds them. */
Eft043Offset eft043_offsets[2] = {
    Eft043Offset(-10.0f, -17.0f, 87.0f),
    Eft043Offset(-10.0f, -17.0f, 45.0f),
};

extern "C" void eft043_release(_EFT* self);
extern "C" void eft043_dispatch(_EFT* self);
extern "C" void eft043_start(_EFT* self);
extern "C" void eft043_move(_EFT* self);
extern "C" void eft043_end(_EFT* self);
extern "C" void eft043_retire(_EFT* self);

/* 0x8031D294 (0xA8): spawns the glow effect of `type` on the player `plw`. */
extern "C" void eft043_set(_PLW* plw, u8 type) {
    _EFT* self = (_EFT*)eft_res_slot_get(0x14);

    if (self != NULL) {
        ((Eft043Work*)self->work_0x38)->count = eft043_effect_counts[type];
        self->field_0x03 = 0x2B;
        self->area_0x44 = plw->area_0x16;
        self->type_0x02 = type;
        self->source_0x30 = plw;
        self->timer_0x0C = 0;
        eft_state_flags_set(self, 1, 0);
        self->release_0x40 = eft043_release;
        self->dispatch_0x34 = eft043_dispatch;
    }
}

/* 0x8031D33C (0x3C): returns the pool's effects to the heap. */
extern "C" void eft043_release(_EFT* self) {
    Eft043Work* work = (Eft043Work*)self->work_0x38;

    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* 0x8031D378 (0x3C): runs the effect's state. */
extern "C" void eft043_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        eft043_start(self);
        return;
    case 1:
        eft043_move(self);
        return;
    case 2:
        eft043_end(self);
        return;
    case 3:
        eft043_retire(self);
        return;
    }
}

/* 0x8031D3B4 (0x280): creates the pool's effects, places type 0 on the player's joint 7 and steps once. */
extern "C" void eft043_start(_EFT* self) {
    s32 i;
    Eft043Work* work = (Eft043Work*)self->work_0x38;
    _PLW* plw = (_PLW*)self->source_0x30;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 dir;

    MTX34_ctor(&mtx);
    VEC3_ctor(&ofs);
    VEC3_ctor(&dir);
    switch (self->type_0x02) {
    case 0:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(eft043_effect_ids[i], 21, 0);
            if (work->effects[i] == NULL) {
                eft043_retire(self);
                return;
            }
        }
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        mhchar_joint_mtx_get(&((Eft043PlwJoints*)plw->physics_0x13C)->joint_0x004, 7, &mtx);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        dir.x = -mtx.m[0][2];
        dir.y = -mtx.m[1][2];
        dir.z = -mtx.m[2][2];
        calcVecAngXY(&dir, &self->rot_0x24.x, &self->rot_0x24.y);
        self->rot_0x24.z = 0;
        copyVec3(&ofs, &eft043_offsets[self->type_0x02]);
        rotVecX(&ofs, self->rot_0x24.x);
        rotVecY(&ofs, self->rot_0x24.y);
        addVec3To(&self->pos_0x18, &ofs);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        if (self->area_0x44 == (u8)get_now_areano()) {
            if (plw->kind_0x09 != 3) {
                pl_ryugeki_se_req(plw, &self->pos_0x18, 0);
            } else {
                pl_ryugeki_se_req(plw, &self->pos_0x18, 1);
            }
        }
        work->color.r = eft043_color_keys[1];
        work->color.g = eft043_color_keys[2];
        work->color.b = eft043_color_keys[3];
        work->color.a = 0xFF;
        break;
    case 1:
        for (i = 0; i < work->count; i++) {
            work->effects[i] = res_eft_create(0x1CE, 21, 0);
            if (work->effects[i] == NULL) {
                eft043_retire(self);
                return;
            }
        }
        break;
    }
    self->state_0x05++;
    self->field_0x06 = 0;
    work->scale = 1.0f;
    self->flag_0x01 = 1;
    eft043_move(self);
}

/* 0x8031D634 (0x35C): follows the player's joint 7, runs the colour and scale keys, and ends the effect when the
 * player leaves the glowing act. */
extern "C" void eft043_move(_EFT* self) {
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 scale;
    _PLW* plw;
    Eft043Work* work;
    s32 i;
    nw4r::ef::EffectSystem* system;

    MTX34_ctor(&mtx);
    VEC3_ctor(&ofs);
    VEC3_ctor(&dir);
    VEC3_ctor(&scale);
    plw = (_PLW*)self->source_0x30;
    work = (Eft043Work*)self->work_0x38;
    system = eft_control.effect_system_0x04;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->timer_0x0C = 0;
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->type_0x02 == 0) {
        if (pl_act4_phase_ck(plw, 0) == 0) {
            if (pl_act4_phase_ck(plw, 1) == 1) {
                self->timer_0x0C = 6;
            } else {
                self->timer_0x0C = 0;
            }
            self->state_0x05++;
            RetireParticleAll(work->effects[1]);
            work->effects[1]->RetireEmitterAll();
            if (system != NULL) {
                system->RetireEffect(work->effects[1]);
                work->count--;
            }
            eft043_end(self);
            return;
        }
    } else if (pl_act4_phase_ck(plw, 0) == 0) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }
    mhchar_joint_mtx_get(&((Eft043PlwJoints*)plw->physics_0x13C)->joint_0x004, 7, &mtx);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    dir.x = -mtx.m[0][2];
    dir.y = -mtx.m[1][2];
    dir.z = -mtx.m[2][2];
    calcVecAngXY(&dir, &self->rot_0x24.x, &self->rot_0x24.y);
    self->rot_0x24.z = 0;
    copyVec3(&ofs, &eft043_offsets[self->type_0x02]);
    rotVecX(&ofs, self->rot_0x24.x);
    rotVecY(&ofs, self->rot_0x24.y);
    addVec3To(&self->pos_0x18, &ofs);
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    mtx34_set_trans(&mtx, &self->pos_0x18);
    switch (self->type_0x02) {
    case 0:
        self->timer_0x0C++;
        eftGetKeyRGB(eft043_color_keys, self->timer_0x0C, &work->color.r, &work->color.g, &work->color.b);
        getKeyData3(eft043_scale_keys, (f32)self->timer_0x0C, &scale.x, &scale.y, &scale.z);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            change_paramscale_eff_vec3(work->effects[0], &scale);
            effect_move(work->effects[i]);
        }
        change_color_eff(work->effects[0], &self->pos_0x18, work->color);
        break;
    case 1:
        work->effects[0]->SetRootMtx(mtx);
        change_paramscale_eff(work->effects[0], 0.8f);
        if (effect_move(work->effects[0]) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        break;
    }
    if (Pl_master_ck(plw) == 1) {
        eft_res_models_spawn(self, (void**)work->effects, eft_stage_spawn_flag(plw) | 1, work->count, NULL);
    } else {
        eft_res_models_spawn(self, (void**)work->effects, 1, work->count, NULL);
    }
}

/* 0x8031D990 (0xC0): fades type 0 out over its timer, then lets the record retire. */
extern "C" void eft043_end(_EFT* self) {
    Eft043Work* work = (Eft043Work*)self->work_0x38;

    switch (self->type_0x02) {
    case 0:
        if (--self->timer_0x0C >= 0) {
            effect_move(work->effects[0]);
            change_color_eff(work->effects[0], &self->pos_0x18, work->color);
            eft_res_models_spawn(self, (void**)work->effects, 1, 1, NULL);
        } else {
            self->flag_0x01 = 0;
            self->state_0x05++;
        }
        break;
    case 1:
        self->state_0x05++;
        break;
    }
}

/* 0x8031DA50 (0x4): retires the record. */
extern "C" void eft043_retire(_EFT* self) {
    eft_res_slot_release(self);
}
