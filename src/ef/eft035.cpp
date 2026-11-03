/*
 * ef/eft035.cpp - the eft035 enemy-effect family (`eft035_set`/`eft035_set2`, its `state_0x05` machine
 *   `fn_802F2640` and the per-type bodies).
 * RANGE. .text 0x802F2238-0x802F39DC (17 functions, defined in address order); extab 0x8001547C-0x800154D4,
 *   extabindex 0x80033D20-0x80033DA4, .data 0x805D69A8-0x805D6CE8 and .sdata 0x80792860-0x80792880 (the key and
 *   per-type tables, defined here in emission order), .sdata2 0x8079A9C8-0x8079A9F0 (its literal pool).  The cockpit
 *   HUD functions below 0x802F2238 are `hud/cockpit_quest.cpp`'s; `ef/eft034.cpp` starts at 0x802F39DC.
 * FLAGS. `cflags_main`; `#pragma pool_data off`, `peephole off`, `optimization_level 4`, `fp_contract off` file-wide.
 * NAMES. `eft035_set` and `eft035_set2` are the runtime dump's own names; the map has only `fn_` stems for the rest,
 *   so plain definitions are `extern "C"`.
 *   GUESS (from each body and its callers): `eft035_scale_keys_2`, `eft035_scale_keys_4`, `eft035_scale_keys_5`,
 *   `eft035_scale_keys_6`, `eft035_scale_keys_7`, `eft035_yaw_keys_4` (`_0`..`_2` its per-model rows),
 *   `eft035_yaw_keys_6`, `eft035_yaw_keys_7`, `eft035_alpha_keys_4`, `eft035_alpha_keys_5`, `eft035_color_keys_6`
 *   (`_0`..`_3` its per-sub-model rows), `eft035_color_keys_7` (`_0`, `_1`), `eft035_alpha_keys_2`,
 *   `eft035_model_counts`, `eft035_type_variants`, `eft035_model_growth` (the map's name of the pooled 0.1f).
 * RESIDUALS. none; the dispatcher's case-0 arm is the static `eft035_start` (its own map row
 *   `eft035_start__FP4_EFT`, split off `fn_802F2640`).
 * SHAPES. `eft035_set`/`eft035_set2` take a 64- and a 56-byte work block (`eft_res_slot_get(64)`/`(56)`): 20-byte model
 *   records at work+0x08 for types 0/1/3, 4-byte handles at work+0x08 for types 4-7.
 */

#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft019.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "pl.h"
#include "fn_8004CAD8.h"               /* MTX34_ctor */
#include "Pl/fn_8028F66C.h"            /* copyVec3 */
#include "g3d/g3d_calcworld.h"         /* addVec3To */
#include "sound/fn_800D7F54.h"         /* em015_denki_eft_se_req, se_req_pos_ps */
#include "stage/stg_w.h"               /* get_now_areano */
#include "Runtime.PPCEABI.H/memset.h"  /* memset - owner Runtime.PPCEABI.H/memset.c */
#include "nw4r/g3d/scnmdl.h"             /* nw4r::g3d::ScnMdl::CopiedMatAccess */
#include "g3d/g3d_scnmdl.h"              /* scn_mdl_mat_access_get_tev_color (rule 2) */
#include "g3d/g3d_resmat.h"              /* nw4r::g3d::ResMatTevColor (rule 2) */
#include "g3d/g3d_calcmaterial.h"        /* res_mat_tev_color_copy_ctor / _end_edit (rule 2) */
#include "unsplit/sound.h"                /* fn_800E2994 */
#include "camera/camera.h"               /* get_camera_pos (rule 2) */
#include "ai/fn_802D44F4.h"              /* player_color_get (rule 2) */
#include "stage/get_now_mapno.h"          /* get_now_mapno (rule 2) */
#include "ef/eft001.h"                    /* mtx34_set_trans (rule 2) */
#include "ef/eft004_fx.h"                 /* fn_801006A0 (rule 2) */
#include "Pl/pl_now_area_ck.h"              /* fn_8026FE98 */
#include "Pl/pl_skill.h"                 /* Pl_Skill_ck (rule 2) */
#include "Pl/fn_802693C4.h"              /* Get_motion_no / Pl_frame_check (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * this unit's own bodies, forward-declared (address order)
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_802F24E0(_EFT* self);
extern "C" void fn_802F250C(_EFT* self);
extern "C" void fn_802F25BC(_EFT* self);
extern "C" void fn_802F2640(_EFT* self);
extern "C" void fn_802F26B4(_EFT* self);
extern "C" void fn_802F288C(_EFT* self);
extern "C" void fn_802F2C78(_EFT* self);
extern "C" void fn_802F2988(_EFT* self);
extern "C" void fn_802F2CB0(_EFT* self);
extern "C" void fn_802F31F0(_EFT* self);
extern "C" void fn_802F3358(_EFT* self);
extern "C" void fn_802F3940(_EFT* self);
extern "C" void fn_802F3950(_EFT* self);
extern "C" void fn_802F3954(MHchar* model, u32 visible, u32 from, u32 to);
void eft035_set(_ENEMY_WORK* self, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, f32 scale);
void eft035_set2(_ENEMY_WORK* self, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, f32 scale);

/* ---------------------------------------------------------------------------------------------------
 * the per-family work blocks
 * ------------------------------------------------------------------------------------------------- */

/* One pooled model record of the type-0/1/3 family: the colour step the update walks (`state_0x00`
 * selects the colour), the model handle `eft_res_model_get` pools and `eft_res_models_release` releases, and the
 * model record `res_eft_UV_model_create` returned.  The release and the create both index this array
 * with a stride of 0x14 off work+0x08. size: 0x14 */
struct Eft035Slot {
    /* +0x00 */ u8 state_0x00;       /* the colour step the update advances, 0..3 */
    /* +0x01 */ _GXColor color_0x01; /* the colour `state_0x00` selects */
    /* +0x05 */ u8 unused_0x05[0x07];
    /* +0x0C */ MHchar* model_0x0C;  /* the pooled handle: `eft_res_models_release` releases this word */
    /* +0x10 */ void* created_0x10;  /* what the create returned (the update virtual-calls it) */
};

/* Work block of the type-0/1/3 family (the 64-byte block `eft035_set` pools).  `count_0x00` is the
 * pool's used length, `scale_0x04` the effect's start scale, `slots_0x08` the two 20-byte model
 * records and `works_0x30` the 2x2 `_g3d_work` handle list `res_eft_UV_model_create` fills (one
 * 8-byte pair per model, indexed by the same count as `slots_0x08`).
 * size: 0x40 (the size `eft_res_slot_get(64)` pools) */
struct Eft035Work {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ f32 scale_0x04;
    /* +0x08 */ Eft035Slot slots_0x08[2];
    /* +0x30 */ struct _g3d_work* works_0x30[2][2];
};

/* Work block of the type-4..7 family (the 56-byte block `eft035_set2` pools): a 4-byte-stride model
 * handle array at +0x08 whose length is `count_0x00`, the create's returns at +0x14 and the handle
 * list at +0x20 (one 8-byte pair per model). size: 0x38 (the size `eft_res_slot_get(56)` pools) */
struct Eft035Work2 {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ f32 scale_0x04;
    /* +0x08 */ MHchar* models_0x08[3];
    /* +0x14 */ void* created_0x14[3];
    /* +0x20 */ struct _g3d_work* works_0x20[3][2];
};

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.data`/`.sdata` tables, in retail emission order.  A scale key is {frame, x, y, z}, a yaw key
 * {frame, yaw}, an alpha key {frame, alpha} and a colour key {frame, r, g, b}; frame -1 (0xFF for the byte keys)
 * ends a table.
 * ------------------------------------------------------------------------------------------------- */

#pragma pool_data off /* each table is reached by name, never through a shared .data anchor (playbook 194) */

extern "C" {

/* the type-2 scale keys */
f32 eft035_scale_keys_2[20] = {
    0.0f, 1.0f, 1.4f, 1.0f,
    2.0f, 1.8f, 1.2f, 1.8f,
    6.0f, 2.0f, 0.8f, 2.0f,
    20.0f, 2.2f, 0.6f, 2.2f,
    -1.0f, 2.2f, 0.6f, 2.2f,
};
/* the type-4 scale keys */
f32 eft035_scale_keys_4[20] = {
    0.0f, 0.1f, 1.0f, 0.1f,
    10.0f, 1.3f, 0.8f, 1.3f,
    20.0f, 1.35f, 0.8f, 1.35f,
    35.0f, 2.0f, 1.4f, 2.0f,
    -1.0f, 2.0f, 1.4f, 2.0f,
};
/* the type-5 scale keys */
f32 eft035_scale_keys_5[20] = {
    0.0f, 1.0f, 0.0f, 1.0f,
    2.0f, 1.0f, 1.0f, 1.0f,
    3.0f, 1.0f, 1.0f, 1.0f,
    8.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 0.0f,
};
/* the type-6 scale keys */
f32 eft035_scale_keys_6[28] = {
    0.0f, 0.1f, 0.1f, 0.1f,
    2.0f, 0.1f, 0.1f, 0.1f,
    3.0f, 2.0f, 0.5f, 2.0f,
    8.0f, 6.0f, 3.0f, 6.0f,
    22.0f, 4.0f, 4.0f, 4.0f,
    37.0f, 3.0f, 4.5f, 3.0f,
    -1.0f, 3.0f, 4.5f, 3.0f,
};
/* the type-7 scale keys */
f32 eft035_scale_keys_7[20] = {
    0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 2.0f, 2.0f, 2.0f,
    6.0f, 4.5f, 4.0f, 4.5f,
    36.0f, 5.8f, 3.0f, 5.8f,
    -1.0f, 5.8f, 3.0f, 5.8f,
};
/* the type-4 yaw keys of model 0 */
f32 eft035_yaw_keys_4_0[16] = {
    0.0f, 0.0f,
    10.0f, 142.0f,
    15.0f, 288.0f,
    20.0f, 442.0f,
    25.0f, 494.0f,
    30.0f, 524.0f,
    35.0f, 533.0f,
    -1.0f, 533.0f,
};
/* the type-4 yaw keys of model 1 */
f32 eft035_yaw_keys_4_1[16] = {
    0.0f, 0.0f,
    10.0f, 106.0f,
    15.0f, 216.0f,
    20.0f, 331.5f,
    25.0f, 370.5f,
    30.0f, 393.0f,
    35.0f, 400.0f,
    -1.0f, 400.0f,
};
/* the type-4 yaw keys of model 2 */
f32 eft035_yaw_keys_4_2[16] = {
    0.0f, 0.0f,
    10.0f, 71.0f,
    15.0f, 144.0f,
    20.0f, 221.5f,
    25.0f, 247.5f,
    30.0f, 262.0f,
    35.0f, 266.5f,
    -1.0f, 266.5f,
};
/* the type-4 yaw keys per model */
f32* eft035_yaw_keys_4[4] = { eft035_yaw_keys_4_0, eft035_yaw_keys_4_1, eft035_yaw_keys_4_2, NULL };
/* the type-6 yaw keys */
f32 eft035_yaw_keys_6[6] = {
    0.0f, 0.0f,
    37.0f, 1080.0f,
    -1.0f, 1080.0f,
};
/* the type-7 yaw keys */
f32 eft035_yaw_keys_7[6] = {
    0.0f, 0.0f,
    36.0f, 1080.0f,
    -1.0f, 1080.0f,
};
/* the type-4 alpha keys */
u8 eft035_alpha_keys_4[12] = {
    0x00, 0x00, 0x0A, 0xFF,
    0x14, 0xFF, 0x23, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};
/* the type-5 alpha keys */
u8 eft035_alpha_keys_5[12] = {
    0x00, 0x00, 0x02, 0xFF,
    0x03, 0xFF, 0x08, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};
/* the type-6 colour keys of sub-model 2 */
u8 eft035_color_keys_6_2[16] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x0C, 0xFF, 0xFF, 0xFF,
    0x16, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
/* the type-6 colour keys of sub-model 0 */
u8 eft035_color_keys_6_0[16] = {
    0x00, 0xFF, 0xAF, 0x61, 0x0C, 0xFF, 0xAF, 0x61,
    0x1C, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
/* the type-6 colour keys of sub-model 1 */
u8 eft035_color_keys_6_1[16] = {
    0x00, 0x74, 0x43, 0x12, 0x16, 0x74, 0x43, 0x12,
    0x25, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
/* the type-6 colour keys of sub-model 3 */
u8 eft035_color_keys_6_3[16] = {
    0x00, 0xFF, 0x8D, 0x4F, 0x0C, 0xFF, 0x8D, 0x4F,
    0x1B, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
/* the type-6 sub-model colour keys */
u8* eft035_color_keys_6[4] = { eft035_color_keys_6_0, eft035_color_keys_6_1, eft035_color_keys_6_2, eft035_color_keys_6_3 };
/* the type-7 colour keys of sub-model 0 */
u8 eft035_color_keys_7_0[16] = {
    0x00, 0xD5, 0x74, 0x24, 0x0E, 0xD5, 0x74, 0x24,
    0x21, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
/* the type-7 colour keys of sub-model 1 */
u8 eft035_color_keys_7_1[24] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xA5, 0x5B,
    0x0E, 0xFF, 0xA5, 0x5B, 0x20, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* per-type pool length, both eft035 families */
u8 eft035_model_counts[8] = {
    0x02, 0x02, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01,
};
/* type -> variant index, both eft035 families */
u8 eft035_type_variants[8] = {
    0x00, 0x00, 0x01, 0x00, 0x02, 0x02, 0x02, 0x02,
};
/* the type-2 alpha keys */
u8 eft035_alpha_keys_2[8] = {
    0x00, 0xFF, 0x0F, 0xFF, 0x14, 0x00, 0xFF, 0x00,
};
/* the type-7 sub-model colour keys */
u8* eft035_color_keys_7[2] = { eft035_color_keys_7_0, eft035_color_keys_7_1 };

}

/* The by-value spelling of the out-pointer vector subtraction (the result pointer is the hidden first argument). */
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off

/* Spawns the eft035 effect for a type-0/1/3 variant: builds the record, seeds the pool length from
 * the per-type table, takes one model record per entry, and installs position, rotation and area. */
void eft035_set(_ENEMY_WORK* self, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area,
                f32 scale)
{
    if (area != (u8)get_now_areano()) {
        return;
    }
    _EFT* eft = (_EFT*)eft_res_slot_get(64);
    if (eft == 0) {
        return;
    }
    eft->type_0x02 = type;
    eft->release_0x40 = fn_802F24E0;
    eft->dispatch_0x34 = fn_802F2640;
    Eft035Work* work = (Eft035Work*)eft->work_0x38;
    work->count_0x00 = 1;
    memset(&work->works_0x30[0][0], 0, sizeof(work->works_0x30));
    for (int i = 0; i < eft035_model_counts[type]; i++) {
        work->slots_0x08[i].model_0x0C = (MHchar*)eft_res_model_get();
        if (work->slots_0x08[i].model_0x0C == 0) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->field_0x03 = 35;
    eft->field_0x04 = 0;
    eft->source_0x30 = self;
    eft->field_0x10 = 1;
    eft->timer_0x0C = 0;
    copyVec3(&eft->pos_0x18, pos);
    eft->rot_0x24.x = rot->x;
    eft->rot_0x24.y = rot->y;
    eft->area_0x44 = area;
    eft_state_flags_set(eft, 0, 0);
    ((Eft035Work*)eft->work_0x38)->scale_0x04 = 1.2f * scale;
}

/* Spawns the eft035 effect for a type-4..7 variant: the same record with the 4-byte model handle
 * array at work+0x08 and the three handle pairs at work+0x20. */
void eft035_set2(_ENEMY_WORK* self, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area,
                 f32 scale)
{
    if (area != (u8)get_now_areano()) {
        return;
    }
    _EFT* eft = (_EFT*)eft_res_slot_get(56);
    if (eft == 0) {
        return;
    }
    eft->type_0x02 = type;
    eft->release_0x40 = fn_802F24E0;
    eft->dispatch_0x34 = fn_802F2640;
    Eft035Work2* work = (Eft035Work2*)eft->work_0x38;
    work->count_0x00 = eft035_model_counts[type];
    memset(&work->works_0x20[0][0], 0, 24);
    for (int i = 0; i < work->count_0x00; i++) {
        work->models_0x08[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x08[i] == 0) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->field_0x03 = 35;
    eft->field_0x04 = 0;
    eft->source_0x30 = self;
    eft->field_0x10 = 1;
    eft->timer_0x0C = 0;
    copyVec3(&eft->pos_0x18, pos);
    eft->rot_0x24.x = rot->x;
    eft->rot_0x24.y = rot->y;
    eft->area_0x44 = area;
    eft_state_flags_set(eft, 0, 0);
    ((Eft035Work2*)eft->work_0x38)->scale_0x04 = scale;
}

/* The record's pool release: picks the family's release handler from the variant the type table
 * selects. */
extern "C" void fn_802F24E0(_EFT* self)
{
    switch (eft035_type_variants[self->type_0x02]) {
    case 0:
    case 1:
        fn_802F250C(self);
        break;
    case 2:
        fn_802F25BC(self);
        break;
    }
}

/* The record's per-frame dispatcher: `state_0x05` 0 builds the effect through the type switch,
 * 1 runs it, 2 advances the state and 3 tears the record down. */
static void eft035_start(_EFT* self);

/* Releases the type-0/1/3 family's pooled handles: both model pairs at work+0x30 in descending slot
 * order, then the family's pooled model handles one at a time, then clears the pool length. */
extern "C" void fn_802F250C(_EFT* self)
{
    int i;
    int j;
    Eft035Work* work = (Eft035Work*)self->work_0x38;

    for (i = 0; i < 2; i++) {
        for (j = 1; j >= 0; j--) {
            struct _g3d_work* handle = work->works_0x30[i][j];
            if (handle != 0) {
                push_g3d_wk(handle);
            }
        }
    }
    for (i = 0; i < eft035_model_counts[self->type_0x02]; i++) {
        eft_res_models_release(&work->slots_0x08[i].model_0x0C, 1);
    }
    work->count_0x00 = 0;
}

/* Releases the type-4..7 family's pooled handles: the three model pairs at work+0x20 in descending
 * slot order, then the whole handle array, then clears the pool length. */
extern "C" void fn_802F25BC(_EFT* self)
{
    int i;
    int j;
    Eft035Work2* work = (Eft035Work2*)self->work_0x38;

    for (i = 0; i < 3; i++) {
        for (j = 1; j >= 0; j--) {
            struct _g3d_work* handle = work->works_0x20[i][j];
            if (handle != 0) {
                push_g3d_wk(handle);
            }
        }
    }
    eft_res_models_release(&work->models_0x08[0], work->count_0x00);
    work->count_0x00 = 0;
}

extern "C" void fn_802F2640(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft035_start(self);
        break;
    case 1:
        fn_802F2C78(self);
        break;
    case 2:
        fn_802F3940(self);
        break;
    case 3:
        fn_802F3950(self);
        break;
    }
}

/* Starts the record by its type's variant. */
static void eft035_start(_EFT* self)
{
    switch (eft035_type_variants[self->type_0x02]) {
    case 0:
        fn_802F26B4(self);
        break;
    case 1:
        fn_802F288C(self);
        break;
    case 2:
        fn_802F2988(self);
        break;
    }
}

/* Builds the family's first model (types 0/1/3): binds the pooled model to the effect's position and rotation,
 * seeds its three random scales and runs the per-type setup. */
extern "C" void fn_802F26B4(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 vec_a;
    nw4r::math::VEC3 vec_b;
    Eft035Work* work = (Eft035Work*)self->work_0x38;

    VEC3_ctor(&vec_a);
    VEC3_ctor(&vec_b);
    MTX34_ctor(&mtx);

    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;

    self->state_0x05++;

    work->slots_0x08[0].created_0x10 = res_eft_UV_model_create(work->slots_0x08[0].model_0x0C, 53, 280, 0,
                                                               (struct _g3d_work**)&work->works_0x30[0][0], 1, 0);
    if (work->slots_0x08[0].created_0x10 == 0) {
        fn_802F3950(self);
        return;
    }

    switch (self->type_0x02) {
    case 0:
        fn_802F3954(work->slots_0x08[0].model_0x0C, 4, 1, 4);
        break;
    case 1:
        em015_denki_eft_se_req(source->se_0xB14, &self->pos_0x18, 3);
        fn_802F3954(work->slots_0x08[0].model_0x0C, 4, 1, 4);
        eft019_set_core(&self->pos_0x18, self->area_0x44, 81, 1.2f, 1.2f);
        break;
    case 3:
        fn_802F3954(work->slots_0x08[0].model_0x0C, 4, 1, 4);
        break;
    }

    copyVec3(&work->slots_0x08[0].model_0x0C->pos_0x04, &self->pos_0x18);
    work->slots_0x08[0].model_0x0C->field_0x28 = (u16)(ran_suu(0) + self->rot_0x24.x);
    work->slots_0x08[0].model_0x0C->field_0x2C = (u16)(ran_suu(0) + self->rot_0x24.y);
    work->slots_0x08[0].model_0x0C->field_0x30 = (u16)(ran_suu(0) + self->rot_0x24.z);
    work->slots_0x08[0].model_0x0C->scale_0x1C.x = work->scale_0x04;
    work->slots_0x08[0].model_0x0C->scale_0x1C.y = work->scale_0x04;
    work->slots_0x08[0].model_0x0C->scale_0x1C.z = work->scale_0x04;

    self->flag_0x01 = 1;
    self->timer_0x0C = 2;
    fn_802F2C78(self);
}

/* Builds the type-2 model: one `res_eft_model_create` model, the family's SE request when the effect
 * is in the current area, then the position and rotation seeds. */
extern "C" void fn_802F288C(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 vec_a;
    nw4r::math::VEC3 vec_b;
    Eft035Work* work = (Eft035Work*)self->work_0x38;

    VEC3_ctor(&vec_a);
    VEC3_ctor(&vec_b);
    MTX34_ctor(&mtx);

    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;

    self->state_0x05++;

    if (res_eft_model_create(work->slots_0x08[0].model_0x0C, 61, 16) == 0) {
        fn_802F3950(self);
        return;
    }
    if (self->area_0x44 == (u8)get_now_areano()) {
        se_req_pos_ps(source->se_0xB14, 31, 2, &self->pos_0x18);
    }
    self->timer_0x0C = 0;
    copyVec3(&work->slots_0x08[0].model_0x0C->pos_0x04, &self->pos_0x18);
    work->slots_0x08[0].model_0x0C->field_0x28 = self->rot_0x24.x;
    work->slots_0x08[0].model_0x0C->field_0x2C = self->rot_0x24.y;
    work->slots_0x08[0].model_0x0C->field_0x30 = self->rot_0x24.z;
    self->flag_0x01 = 1;
    fn_802F2C78(self);
}

/* Builds the type-4..7 models: type 4 one UV model per handle (ids 158 up), type 5 one plain model per handle with its
 * sub-model visibility, types 6 and 7 one UV model (type 6 also spawns its type-7 partner and its sound). */
extern "C" void fn_802F2988(_EFT* self)
{
    Eft035Work2* work = (Eft035Work2*)self->work_0x38;
    nw4r::math::VEC3 vec_a;
    nw4r::math::VEC3 vec_b;
    nw4r::math::MTX34 mtx;
    _ENEMY_WORK* source;
    s32 i;

    VEC3_ctor(&vec_a);
    VEC3_ctor(&vec_b);
    MTX34_ctor(&mtx);
    source = (_ENEMY_WORK*)self->source_0x30;
    self->state_0x05++;
    switch (self->type_0x02) {
    case 4:
        for (i = 0; i < work->count_0x00; i++) {
            work->created_0x14[i] = res_eft_UV_model_create(work->models_0x08[i], (u16)(i + 158), 272, 0,
                                                            (struct _g3d_work**)work->works_0x20[i], 1, 0);
            if (work->created_0x14[i] == 0) {
                fn_802F3950(self);
                return;
            }
            copyVec3(&work->models_0x08[i]->pos_0x04, &self->pos_0x18);
            work->models_0x08[i]->field_0x28 = 0;
            work->models_0x08[i]->field_0x2C = self->rot_0x24.y;
            work->models_0x08[i]->field_0x30 = 0;
        }
        break;
    case 5:
        for (i = 0; i < work->count_0x00; i++) {
            if (res_eft_model_create(work->models_0x08[i], 162, 24) == 0) {
                fn_802F3950(self);
                return;
            }
            copyVec3(&work->models_0x08[i]->pos_0x04, &self->pos_0x18);
            work->models_0x08[i]->field_0x28 = 0;
            work->models_0x08[i]->field_0x2C = self->rot_0x24.y;
            work->models_0x08[i]->field_0x30 = 0;
            work->created_0x14[i] = 0;
            fn_802F3954(work->models_0x08[i], 9, 1, 8);
            work->models_0x08[i]->setVisibility(10, 1);
        }
        break;
    case 6:
        work->created_0x14[0] = res_eft_UV_model_create(work->models_0x08[0], 169, 260, 0,
                                                        (struct _g3d_work**)work->works_0x20[0], 1, 0);
        if (work->created_0x14[0] == 0) {
            fn_802F3950(self);
            return;
        }
        copyVec3(&work->models_0x08[0]->pos_0x04, &self->pos_0x18);
        work->models_0x08[0]->field_0x28 = 0;
        work->models_0x08[0]->field_0x2C = self->rot_0x24.y;
        work->models_0x08[0]->field_0x30 = 0;
        eft035_set2(source, 7, &self->pos_0x18, &self->rot_0x24, self->area_0x44, work->scale_0x04);
        se_req_pos_ps(source->se_0xB14, 156, 2, &self->pos_0x18);
        break;
    case 7:
        work->created_0x14[0] = res_eft_UV_model_create(work->models_0x08[0], 170, 260, 0,
                                                        (struct _g3d_work**)work->works_0x20[0], 1, 0);
        if (work->created_0x14[0] == 0) {
            fn_802F3950(self);
            return;
        }
        copyVec3(&work->models_0x08[0]->pos_0x04, &self->pos_0x18);
        work->models_0x08[0]->field_0x28 = 0;
        work->models_0x08[0]->field_0x2C = self->rot_0x24.y;
        work->models_0x08[0]->field_0x30 = 0;
        break;
    default:
        fn_802F3950(self);
        break;
    }
    self->timer_0x0C = 0;
    self->flag_0x01 = 1;
    fn_802F2C78(self);
}

/* Runs the record's variant update: `fn_802F2CB0`, `fn_802F31F0` and `fn_802F3358` are the three
 * variant bodies the type table selects. */
extern "C" void fn_802F2C78(_EFT* self)
{
    switch (eft035_type_variants[self->type_0x02]) {
    case 0:
        fn_802F2CB0(self);
        break;
    case 1:
        fn_802F31F0(self);
        break;
    case 2:
        fn_802F3358(self);
        break;
    }
}

/* Runs the type-0/1/3 variant: every other frame pools one more UV model (random spin and scale, the type's sub-model
 * set) until the type's count, steps each model's TEV colour through its four shades while it grows, and ends the
 * record once every model has run its colours. */
extern "C" void fn_802F2CB0(_EFT* self)
{
    Eft035Slot* slot;
    s32 i;
    s32 j;
    u8 done = 0;
    Eft035Work* work = (Eft035Work*)self->work_0x38;
    s32 n;

    if (--self->field_0x10 >= 0) {
        return;
    }
    if (--self->timer_0x0C < 0 && (n = work->count_0x00) < eft035_model_counts[self->type_0x02]) {
        work->slots_0x08[work->count_0x00].created_0x10 =
            res_eft_UV_model_create(work->slots_0x08[n].model_0x0C, 53, 280, 0, work->works_0x30[n], 1, 0);
        slot = &work->slots_0x08[work->count_0x00];
        if (slot->created_0x10 == 0) {
            fn_802F3950(self);
            return;
        }
        switch (self->type_0x02) {
        case 0:
            fn_802F3954(slot->model_0x0C, 4, 1, 4);
            break;
        case 1:
            eft019_set_core(&self->pos_0x18, self->area_0x44, 82, 1.2f, 1.2f);
            fn_802F3954(work->slots_0x08[work->count_0x00].model_0x0C, 4, 1, 4);
            break;
        case 3:
            fn_802F3954(slot->model_0x0C, 4, 1, 4);
            break;
        }
        self->timer_0x0C = 2;
        copyVec3(&work->slots_0x08[work->count_0x00].model_0x0C->pos_0x04, &self->pos_0x18);
        work->slots_0x08[work->count_0x00].model_0x0C->field_0x28 = (u16)(self->rot_0x24.x + ran_suu(0));
        work->slots_0x08[work->count_0x00].model_0x0C->field_0x2C = (u16)(self->rot_0x24.y + ran_suu(0));
        work->slots_0x08[work->count_0x00].model_0x0C->field_0x30 = (u16)(self->rot_0x24.z + ran_suu(0));
        work->slots_0x08[work->count_0x00].model_0x0C->scale_0x1C.x =
            work->scale_0x04 + 0.1f * (ran_suu(0) & 3);
        work->slots_0x08[work->count_0x00].model_0x0C->scale_0x1C.y =
            work->scale_0x04 + 0.1f * (ran_suu(0) & 3);
        work->slots_0x08[work->count_0x00].model_0x0C->scale_0x1C.z =
            work->scale_0x04 + 0.1f * (ran_suu(0) & 3);
        work->count_0x00++;
    }
    for (i = 0; i < work->count_0x00; i++) {
        if (work->slots_0x08[i].state_0x00 >= 3) {
            done++;
        }
    }
    if (done >= eft035_model_counts[self->type_0x02]) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    for (i = 0; i < work->count_0x00; i++) {
        switch (work->slots_0x08[i].state_0x00) {
        case 0:
            work->slots_0x08[i].color_0x01.r = 0xB4;
            work->slots_0x08[i].color_0x01.g = 0xE5;
            work->slots_0x08[i].color_0x01.b = 0xFF;
            work->slots_0x08[i].color_0x01.a = 0xFF;
            work->slots_0x08[i].state_0x00++;
            break;
        case 1:
            work->slots_0x08[i].color_0x01.r = 0x68;
            work->slots_0x08[i].color_0x01.g = 0x99;
            work->slots_0x08[i].color_0x01.b = 0xF2;
            work->slots_0x08[i].color_0x01.a = 0xFF;
            work->slots_0x08[i].state_0x00++;
            break;
        case 2:
            work->slots_0x08[i].color_0x01.r = 0x22;
            work->slots_0x08[i].color_0x01.g = 0x39;
            work->slots_0x08[i].color_0x01.b = 0x96;
            work->slots_0x08[i].color_0x01.a = 0xFF;
            work->slots_0x08[i].state_0x00++;
            break;
        case 3:
            work->slots_0x08[i].color_0x01.r = 0;
            work->slots_0x08[i].color_0x01.g = 0;
            work->slots_0x08[i].color_0x01.b = 0;
            work->slots_0x08[i].color_0x01.a = 0;
            work->slots_0x08[i].state_0x00++;
            break;
        }
        work->slots_0x08[i].model_0x0C->scale_0x1C.x += 0.1f;
        work->slots_0x08[i].model_0x0C->scale_0x1C.y += 0.1f;
        work->slots_0x08[i].model_0x0C->scale_0x1C.z += 0.1f;
        for (j = 0; j < 4; j++) {
            nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)work->slots_0x08[i].model_0x0C->field_0x118, j);
            if (copied_mat_access_valid_ck(&access) != 0) {
                void* tev;
                u32 handle = scn_mdl_mat_access_get_tev_color(&access, false);
                res_mat_tev_color_copy_ctor((struct ResHandle*)&tev, (struct ResHandle*)&handle);
                ((nw4r::g3d::ResMatTevColor*)&tev)->GXSetTevKColor((_GXTevKColorID)3, work->slots_0x08[i].color_0x01);
                res_mat_tev_color_end_edit((struct ResHandle*)&tev);
            }
        }
        work->slots_0x08[i].model_0x0C->move(0);
        ((EftUvModelObject*)work->slots_0x08[i].created_0x10)->update();
        eft_res_models_spawn(self, (void**)&work->slots_0x08[i].model_0x0C, 2, 1, 0);
    }
}

/* Runs the type-2 variant for 20 frames: each model takes the scale keys times the record's scale and the alpha keys
 * as its TEV constant alpha. */
extern "C" void fn_802F31F0(_EFT* self)
{
    s32 i;
    Eft035Work* work = (Eft035Work*)self->work_0x38;
    f32 x;
    f32 y;
    f32 z;
    _GXColor col;

    if (++self->timer_0x0C > 20) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    for (i = 0; i < work->count_0x00; i++) {
        getKeyData3(eft035_scale_keys_2, self->timer_0x0C, &x, &y, &z);
        work->slots_0x08[i].model_0x0C->scale_0x1C.x = x * work->scale_0x04;
        work->slots_0x08[i].model_0x0C->scale_0x1C.y = y * work->scale_0x04;
        work->slots_0x08[i].model_0x0C->scale_0x1C.z = z * work->scale_0x04;
        work->slots_0x08[i].model_0x0C->move(0);
        col.b = 0xFF;
        col.g = 0xFF;
        col.r = 0xFF;
        col.a = eftGetKeyAlpha(eft035_alpha_keys_2, self->timer_0x0C);
        work->slots_0x08[i].model_0x0C->setTevKColor(0, (_GXTevKColorID)3, &col);
        eft_res_models_spawn(self, (void**)&work->slots_0x08[i].model_0x0C, 2, 1, 0);
    }
}

/* Runs the type-4..7 variant for its frame count: each model takes the type's scale keys times the record's scale,
 * types 4, 6 and 7 a yaw from the rotation keys, and its sub-models the type's alpha (4, 5) or colour keys (6, 7);
 * type 4 hides its last model when the camera is close, type 6 spawns its landing burst on frame 32. */
extern "C" void fn_802F3358(_EFT* self)
{
    nw4r::math::VEC3 cam;
    nw4r::math::VEC3 cam_pos;
    nw4r::math::VEC3 diff;
    s32 i;
    s32 sub_from;
    s32 sub_to;
    s32 count;
    f32* scale_keys;
    u8* alpha_keys;
    Eft035Work2* work;
    s32 j;
    f32 x;
    f32 y;
    f32 z;
    _GXColor col;

    VEC3_ctor(&cam);
    VEC3_ctor(&cam_pos);
    VEC3_ctor(&diff);
    work = (Eft035Work2*)self->work_0x38;
    self->timer_0x0C++;
    switch (self->type_0x02) {
    case 4:
        if (self->timer_0x0C > 35) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        scale_keys = eft035_scale_keys_4;
        alpha_keys = eft035_alpha_keys_4;
        sub_from = 0;
        sub_to = 2;
        copyVec3(&cam_pos, &get_camera_pos());
        copyVec3(&diff, &VEC3_SUB(&self->pos_0x18, &cam_pos));
        if (vec3_len((f32*)&diff) > 1000.0f) {
            count = work->count_0x00;
        } else {
            count = work->count_0x00 - 1;
        }
        break;
    case 5:
        if (self->timer_0x0C > 8) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        scale_keys = eft035_scale_keys_5;
        alpha_keys = eft035_alpha_keys_5;
        sub_from = 1;
        sub_to = 2;
        count = work->count_0x00;
        break;
    case 6:
        if (self->timer_0x0C > 37) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        if (self->timer_0x0C == 32) {
            eft019_set_tyakudan(&self->pos_0x18, &self->rot_0x24, self->area_0x44, 135, 1.0f);
        }
        scale_keys = eft035_scale_keys_6;
        alpha_keys = NULL;
        sub_from = 0;
        sub_to = 4;
        count = work->count_0x00;
        break;
    case 7:
        if (self->timer_0x0C > 36) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        scale_keys = eft035_scale_keys_7;
        alpha_keys = NULL;
        sub_from = 0;
        sub_to = 2;
        count = work->count_0x00;
        break;
    }
    for (i = 0; i < count; i++) {
        getKeyData3(scale_keys, self->timer_0x0C, &x, &y, &z);
        work->models_0x08[i]->scale_0x1C.x = x * work->scale_0x04;
        work->models_0x08[i]->scale_0x1C.y = y * work->scale_0x04;
        work->models_0x08[i]->scale_0x1C.z = z * work->scale_0x04;
        switch (self->type_0x02) {
        case 4:
            work->models_0x08[i]->field_0x2C =
                self->rot_0x24.y + (u16)(0.5f + 65536.0f * getKeyData(eft035_yaw_keys_4[i], self->timer_0x0C) / 360.0f);
            work->models_0x08[i]->move(0);
            if (work->created_0x14[i] != NULL) {
                ((EftUvModelObject*)work->created_0x14[i])->update();
            }
            for (j = sub_from; j < sub_to; j++) {
                work->models_0x08[i]->getTevKColor(j, (_GXTevKColorID)3, &col);
                col.a = eftGetKeyAlpha(alpha_keys, self->timer_0x0C);
                work->models_0x08[i]->setTevKColor(j, (_GXTevKColorID)3, &col);
            }
            break;
        case 5:
            work->models_0x08[i]->move(0);
            for (j = sub_from; j < sub_to; j++) {
                work->models_0x08[i]->getTevKColor(j, (_GXTevKColorID)3, &col);
                col.a = eftGetKeyAlpha(alpha_keys, self->timer_0x0C);
                work->models_0x08[i]->setTevKColor(j, (_GXTevKColorID)3, &col);
            }
            break;
        case 6:
            work->models_0x08[i]->field_0x2C =
                self->rot_0x24.y + (u16)(0.5f + 65536.0f * getKeyData(eft035_yaw_keys_6, self->timer_0x0C) / 360.0f);
            work->models_0x08[i]->move(0);
            ((EftUvModelObject*)work->created_0x14[i])->update();
            for (j = sub_from; j < sub_to; j++) {
                eftGetKeyRGB(eft035_color_keys_6[j], self->timer_0x0C, &col.r, &col.g, &col.b);
                col.a = 0xFF;
                work->models_0x08[i]->setMatColor(j, (_GXChannelID)4, col, 0);
            }
            break;
        case 7:
            work->models_0x08[i]->field_0x2C =
                self->rot_0x24.y + (u16)(0.5f + 65536.0f * getKeyData(eft035_yaw_keys_7, self->timer_0x0C) / 360.0f);
            work->models_0x08[i]->move(0);
            ((EftUvModelObject*)work->created_0x14[i])->update();
            for (j = sub_from; j < sub_to; j++) {
                eftGetKeyRGB(eft035_color_keys_7[j], self->timer_0x0C, &col.r, &col.g, &col.b);
                col.a = 0xFF;
                work->models_0x08[i]->setMatColor(j, (_GXChannelID)4, col, 0);
            }
            break;
        }
        eft_res_models_spawn(self, (void**)&work->models_0x08[i], 2, 1, 0);
    }
}

/* Advances the record's state index. */
extern "C" void fn_802F3940(_EFT* self)
{
    self->state_0x05++;
}

/* Retires the record once the effect is over. */
extern "C" void fn_802F3950(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Makes exactly one model of a joint range visible: `visible` is the joint the effect currently
 * rides, every other joint in [`from`, `to`] is hidden. */
extern "C" void fn_802F3954(MHchar* model, u32 visible, u32 from, u32 to)
{
    for (u32 joint = from; joint <= to; joint++) {
        if (joint == visible) {
            model->setVisibility(joint, true);
        } else {
            model->setVisibility(joint, false);
        }
    }
}
