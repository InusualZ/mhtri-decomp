/*
 * ef/eft035.cpp - the eft035 enemy-effect family (`eft035_set`/`eft035_set2`, its `state_0x05` machine
 *   `fn_802F2640` and the per-type bodies) and a second family with tag 34 (`fn_802F39DC` seeds `field_0x03 = 34`,
 *   `fn_802F3B0C` is its machine).
 * RANGE. .text 0x802F2238-0x802F5138 (28 functions); extab 0x8001547C-0x80015514, extabindex 0x80033D20-0x80033E04,
 *   .data 0x805D69A8-0x805D6D10, .sdata 0x80792860-0x80792880 (the per-type property tables), .sdata2
 *   0x8079A9C8-0x8079AA88.  The cockpit HUD functions below 0x802F2238 are `hud/cockpit_quest.cpp`'s.  The two
 *   families share no call edge, so the range may hold more than one original TU.
 * FLAGS. `cflags_main`; `#pragma peephole off` from `fn_802F250C` to the end of the file.
 * NAMES. `eft035_set` and `eft035_set2` are the runtime dump's own names; the map has only `fn_` stems for the rest,
 *   so plain definitions are `extern "C"`.
 *   GUESS (from each body and its callers): `eft035_ray_flicker`, `eft035_ray_origin_get`, `eft035_throw_start`,
 *   `eft035_orb_move`, `eft035_throw_act_ck`, `eft035_scale_keys_2`, `eft035_scale_keys_4`, `eft035_scale_keys_5`,
 *   `eft035_scale_keys_6`, `eft035_scale_keys_7`, `eft035_yaw_keys_4`, `eft035_yaw_keys_6`, `eft035_yaw_keys_7`,
 *   `eft035_alpha_keys_4`, `eft035_alpha_keys_5`, `eft035_color_keys_6`, `eft035_ray_tip_offsets`,
 *   `eft035_alpha_keys_2`, `eft035_color_keys_7`, `eft035_model_growth`.
 * RESIDUALS. none in code; the dispatcher's case-0 arm is the static `eft035_start` (its own map row
 *   `eft035_start__FP4_EFT`, split off `fn_802F2640`).
 *   flipcheck: `.data` (0x805D69A8-0x805D6D10) is declared, not emitted (its `lbl_805D6xxx` tables are undefined at
 *   link), and `.sdata2` holds several pools (0x8079A9C8.., 0x8079A9F0.., duplicate magic doubles at 0x8079A9D0 and
 *   0x8079AA48), so the range is more than one original TU.
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

extern "C" void fn_802F250C(_EFT* self);
extern "C" void fn_802F25BC(_EFT* self);
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
extern "C" void fn_802F3AD0(_EFT* self);
extern "C" void fn_802F3B0C(_EFT* self);
extern "C" void fn_802F3B48(_EFT* self);
extern "C" void fn_802F3D94(_EFT* self);
extern "C" void fn_802F49C8(_EFT* self);
extern "C" void fn_802F49D8(_EFT* self);
extern "C" void eft035_ray_flicker(_EFT* self);
extern "C" void eft035_ray_origin_get(_EFT* self, nw4r::math::VEC3* out);
extern "C" void eft035_throw_start(_EFT* self, u8 phase);
extern "C" void eft035_orb_move(_EFT* self);
extern "C" u32 eft035_throw_act_ck(_EFT* self);
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

/* Work block of the tag-34 family `fn_802F39DC` seeds (the 76-byte block `eft_res_slot_get(76)` pools): the orb and
 * ray models, the orb's velocity and throw target, its timers and the floor it lands on. size: 0x4C */
struct Eft034Work {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[2];  /* the orb, then the ray */
    /* +0x0C */ nw4r::math::VEC3 vel_0x0C;
    /* +0x18 */ nw4r::math::VEC3 target_0x18; /* where the throw carries the orb */
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ s32 timer_0x30;          /* the phase's frame count (the hit count while it hovers) */
    /* +0x34 */ s32 ray_timer_0x34;      /* the ray's frame count */
    /* +0x38 */ f32 gravity_0x38;
    /* +0x3C */ f32 scale_0x3C;          /* the orb's scale */
    /* +0x40 */ u8 scaled_0x40;          /* the throw keeps its full speed */
    /* +0x41 */ u8 phase_0x41;           /* the throw phase, 0..2 */
    /* +0x42 */ u8 ray_on_0x42;          /* the ray shows this frame */
    /* +0x43 */ u8 stage_0x43;           /* the player's charge stage last frame */
    /* +0x44 */ s32 jitter_0x44;         /* frames until the hovering orb jitters */
    /* +0x48 */ f32 floor_0x48;          /* the floor height of the area */
};

/* The player byte the orb reads that `Pl/plw.h` keeps inside a padding run: the charge stage of the orb throw.
 * size: 0x598 - lower bound, an approximation (a view of `_PLW`) */
struct _EFT035_PL_CHARGE {
    /* +0x000 */ u8 pad_0x000[0x597];
    /* +0x597 */ u8 stage_0x597;
};

/* The player's physics block (`_PLW::physics_0x13C`): the joint character sits at +0x04.  size: 0x144 */
struct _EFT035_PL_PHYS {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr;
};

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata`/`.sdata2` pool, referenced by name and never defined (playbook 29)
 * ------------------------------------------------------------------------------------------------- */

extern "C" u8 lbl_80792860[8]; /* per-type pool length, both eft035 families */
extern "C" const u8 lbl_80792868[8]; /* type -> variant index, both eft035 families */
extern "C" f32 lbl_8079A9C8;         /* the scale factor eft035_set multiplies its argument by */
extern "C" f32 eft035_model_growth;         /* the per-frame growth of the type-0/1/3 models */
extern "C" f32 eft035_scale_keys_2[];        /* the type-2 scale keys (.data 0x805D69A8) */
extern "C" u8 eft035_alpha_keys_2[8];       /* the type-2 alpha keys (.sdata 0x80792870) */
extern "C" u8* eft035_color_keys_7[2];      /* the type-7 sub-model colour keys (.sdata 0x80792878) */
extern "C" f32 eft035_scale_keys_4[];        /* the type-4 scale keys (.data) */
extern "C" f32 eft035_scale_keys_5[];
extern "C" f32 eft035_scale_keys_6[];
extern "C" f32 eft035_scale_keys_7[];
extern "C" f32* eft035_yaw_keys_4[];       /* the type-4 yaw keys per model (.data) */
extern "C" f32 eft035_yaw_keys_6[];        /* the type-6 yaw keys (.data) */
extern "C" f32 eft035_yaw_keys_7[];
extern "C" u8 eft035_alpha_keys_4[];         /* the type-4 alpha keys (.data) */
extern "C" u8 eft035_alpha_keys_5[];
extern "C" u8* eft035_color_keys_6[];        /* the type-6 sub-model colour keys (.data) */
extern "C" Vec eft035_ray_tip_offsets[3];       /* the tag-34 ray's tip offsets per swing pose (.data) */

/* The by-value spelling of the out-pointer vector subtraction (the result pointer is the hidden first argument). */
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)

/* `eft004_key_spawn` (`ef/eft004_fx.cpp`'s ground burst) with its scale third: every caller loads f1 between r4 and r5. */
#define EFT_GROUND_BURST ((void (*)(u8, nw4r::math::VEC3*, f32, u32, u8, u8))eft004_key_spawn)

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off

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
    for (i = 0; i < lbl_80792860[self->type_0x02]; i++) {
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

/* The record's pool release: picks the family's release handler from the variant the type table
 * selects. */
extern "C" void fn_802F24E0(_EFT* self)
{
    switch (lbl_80792868[self->type_0x02]) {
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
    switch (lbl_80792868[self->type_0x02]) {
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

/* Runs the record's variant update: `fn_802F2CB0`, `fn_802F31F0` and `fn_802F3358` are the three
 * variant bodies the type table selects. */
extern "C" void fn_802F2C78(_EFT* self)
{
    switch (lbl_80792868[self->type_0x02]) {
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
        eft019_set_core(&self->pos_0x18, self->area_0x44, 81, lbl_8079A9C8, lbl_8079A9C8);
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
    if (--self->timer_0x0C < 0 && (n = work->count_0x00) < lbl_80792860[self->type_0x02]) {
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
            eft019_set_core(&self->pos_0x18, self->area_0x44, 82, lbl_8079A9C8, lbl_8079A9C8);
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
            work->scale_0x04 + eft035_model_growth * (ran_suu(0) & 3);
        work->slots_0x08[work->count_0x00].model_0x0C->scale_0x1C.y =
            work->scale_0x04 + eft035_model_growth * (ran_suu(0) & 3);
        work->slots_0x08[work->count_0x00].model_0x0C->scale_0x1C.z =
            work->scale_0x04 + eft035_model_growth * (ran_suu(0) & 3);
        work->count_0x00++;
    }
    for (i = 0; i < work->count_0x00; i++) {
        if (work->slots_0x08[i].state_0x00 >= 3) {
            done++;
        }
    }
    if (done >= lbl_80792860[self->type_0x02]) {
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

/* Spawns the tag-34 effect: pools two model handles, seeds the family's zeroed position/rotation and
 * clock, then installs the two hooks that travel with the record. */
extern "C" void fn_802F39DC(_ENEMY_WORK* self, u8 type)
{
    _EFT* eft;
    s32 i;
    Eft034Work* work;

    eft = (_EFT*)eft_res_slot_get(76);
    if (eft == 0) {
        return;
    }
    eft->release_0x40 = fn_802F3AD0;
    work = (Eft034Work*)eft->work_0x38;
    eft->type_0x02 = type;
    work->count_0x00 = 2;
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x04[i] == 0) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
    eft_state_flags_set(eft, 1, 0);
    eft->field_0x03 = 34;
    eft->area_0x44 = self->field_0x016;
    eft->flag_0x01 = 1;
    eft->source_0x30 = self;
    eft->dispatch_0x34 = fn_802F3B0C;
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

/* Releases the tag-34 family's pooled model handles and clears the pool length. */
extern "C" void fn_802F3AD0(_EFT* self)
{
    Eft034Work* work = (Eft034Work*)self->work_0x38;

    eft_res_models_release(&work->models_0x04[0], work->count_0x00);
    work->count_0x00 = 0;
}

/* The tag-34 record's per-frame dispatcher: `state_0x05` 0 builds, 1 runs, 2 advances and 3 retires
 * the record. */
extern "C" void fn_802F3B0C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_802F3B48(self);
        break;
    case 1:
        fn_802F3D94(self);
        break;
    case 2:
        fn_802F49C8(self);
        break;
    case 3:
        fn_802F49D8(self);
        break;
    }
}

/* Builds the tag-34 effect: the orb model (its sub-model by the player's act, the player's colour), the ray model, a
 * random launch velocity along the player's facing, the orb's scale and the floor height of the map. */
extern "C" void fn_802F3B48(_EFT* self)
{
    s32 i;
    _PLW* plw = (_PLW*)self->source_0x30;
    Eft034Work* work = (Eft034Work*)self->work_0x38;
    _GXColor col;
    u32 c;

    self->state_0x05++;
    if (res_eft_model_create(work->models_0x04[0], 50, 24) == 0) {
        fn_802F49D8(self);
        return;
    }
    work->models_0x04[0]->ready = 0;
    if (plw->field_0x306 == 206) {
        work->models_0x04[0]->setVisibility(2, 1);
        work->models_0x04[0]->setVisibility(1, 0);
        self->type_0x02 = 1;
    } else {
        work->models_0x04[0]->setVisibility(2, 0);
        work->models_0x04[0]->setVisibility(1, 1);
    }
    c = player_color_get(plw->chunk_ofs);
    col.r = c >> 24;
    col.g = (c >> 16) & 0xFF;
    col.b = (c >> 8) & 0xFF;
    col.a = c;
    for (i = 0; i < 2; i++) {
        work->models_0x04[0]->setTevKColor(i, (_GXTevKColorID)3, &col);
    }
    if (res_eft_model_create(work->models_0x04[1], 49, 16) == 0) {
        fn_802F49D8(self);
        return;
    }
    setVector3(&work->models_0x04[1]->scale_0x1C, 1.0f, 1.0f, 1.0f);
    ((_EFT035_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
    self->rot_0x24.y = plw->field_0x058;
    work->vel_0x0C.x = eft_rand_signed_half();
    work->vel_0x0C.y = 2.0f + 2.0f * eft_rand_unit();
    work->vel_0x0C.z = 30.0f + 2.0f * eft_rand_unit();
    work->gravity_0x38 = -2.0f;
    rotVecY(&work->vel_0x0C, self->rot_0x24.y);
    if ((s32)self->type_0x02 != 1) {
        work->scale_0x3C = 1.0f;
    } else {
        work->scale_0x3C = 0.3f;
    }
    setVector3(&work->models_0x04[0]->scale_0x1C, work->scale_0x3C, work->scale_0x3C, work->scale_0x3C);
    switch (get_now_mapno()) {
    case 4:
    case 15:
        if ((s32)self->area_0x44 == 0) {
            work->floor_0x48 = -1170.0f;
        } else {
            work->floor_0x48 = 0.0f;
        }
        break;
    default:
        work->floor_0x48 = 0.0f;
        break;
    }
    fn_802F3D94(self);
}

/* Runs the tag-34 orb by its phase: it falls to the floor and lands (0), waits for the charge act (1), hovers and takes
 * the player's hits while the throw motions start their phases (2), wanders around its anchor (3), flies its throw (4)
 * and rides the weapon back (5); then it takes the player's colour (grey while hit) and the ray, and spawns. */
extern "C" void fn_802F3D94(_EFT* self)
{
    _PLW* plw;
    nw4r::math::VEC3 wander;
    nw4r::math::VEC3 fx;
    Eft034Work* work;
    _GXColor col;
    s32 hits;
    s32 frames;
    f32 target_scale;
    f32 dist;
    u32 c;
    u32 motion;
    s32 i;
    s32 need;

    plw = (_PLW*)self->source_0x30;
    VEC3_ctor(&wander);
    VEC3_ctor(&fx);
    work = (Eft034Work*)self->work_0x38;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->timer_0x0C++;
    switch (self->field_0x06) {
    case 0:
        eft035_orb_move(self);
        if (self->pos_0x18.y < work->floor_0x48 - 20.0f) {
            self->pos_0x18.y = work->floor_0x48 - 20.0f;
            copyVec3(&work->target_0x18, &self->pos_0x18);
            work->vel_0x0C.x = 0.0f;
            work->vel_0x0C.y = 0.0f;
            work->vel_0x0C.z = 0.0f;
            self->timer_0x0C = 0;
            self->field_0x06++;
            copyVec3(&fx, &work->target_0x18);
            fx.y = 5.0f + work->floor_0x48;
            EFT_GROUND_BURST(2, &fx, 0.6f, 0, plw->area_0x16, 1);
            EFT_GROUND_BURST(1, &fx, 0.6f, 0, plw->area_0x16, 1);
            if (self->area_0x44 == get_now_areano()) {
                se_req_orb(0, &fx);
            }
        }
        break;
    case 1:
        if (pl_status_bit_ck((struct _ENEMY_WORK*)plw, 0x800) == 0) {
            self->state_0x05++;
            return;
        }
        if (Get_motion_no(plw) == 339) {
            self->field_0x06++;
        }
        break;
    case 2:
        if (pl_status_bit_ck((struct _ENEMY_WORK*)plw, 0x800) == 0) {
            self->state_0x05++;
            return;
        }
        if (work->timer_0x30 < plw->field_0x59A || ((_EFT035_PL_CHARGE*)plw)->stage_0x597 > work->stage_0x43) {
            copyVec3(&fx, &work->target_0x18);
            copyVec3(&self->pos_0x18, &fx);
            fx.y = 5.0f + work->floor_0x48;
            EFT_GROUND_BURST(2, &fx, 1.2f, 0, plw->area_0x16, 1);
            if (self->area_0x44 == get_now_areano()) {
                se_req_orb(1, &fx);
            }
            hits = plw->field_0x59A;
            if (hits > 20) {
                hits = 20;
            }
            work->vel_0x0C.y = -(20.0f + hits);
            self->demo_flag_0x08 |= 0x80;
        }
        work->timer_0x30 = plw->field_0x59A;
        if (work->timer_0x30 <= 0) {
            self->demo_flag_0x08 &= 0x7F;
        }
        if (self->pos_0x18.y >= work->target_0x18.y) {
            self->pos_0x18.y = work->target_0x18.y;
            work->vel_0x0C.y *= 0.2f;
            work->gravity_0x38 = 2.0f;
        } else {
            work->gravity_0x38 = 1.0f;
        }
        if (((_EFT035_PL_CHARGE*)plw)->stage_0x597 != 0 && eft035_throw_act_ck(self) == 1) {
            if (work->jitter_0x44 < 0 && self->pos_0x18.y >= work->target_0x18.y - 2.0f) {
                if (--self->field_0x10 > 0) {
                    self->pos_0x18.x = work->target_0x18.x - 1.0f + (ran_suu(0) & 3);
                    self->pos_0x18.z = work->target_0x18.z - 1.0f + (ran_suu(0) & 3);
                } else {
                    work->jitter_0x44 = (ran_suu(0) + 20) & 0xF;
                    self->field_0x10 = 10;
                }
            } else {
                work->jitter_0x44--;
            }
        }
        switch (Get_motion_no(plw)) {
        case 340:
            self->demo_flag_0x08 &= 0x7F;
            self->field_0x06++;
            work->vel_0x0C.x = self->pos_0x18.x - plw->vec_0x03C.x;
            work->vel_0x0C.z = self->pos_0x18.z - plw->vec_0x03C.z;
            vec3_normalize_into(&work->vel_0x0C, &work->vel_0x0C);
            switch (plw->armed_motion_0x59C) {
            case 210:
            case 211:
            case 215:
                work->scaled_0x40 = 0;
                break;
            default:
                vec3_scale_in_place(&work->vel_0x0C, 3.0f);
                work->scaled_0x40 = 1;
                break;
            }
            rotVecY(&work->vel_0x0C, (ran_suu(0) & 0x3F) - 32);
            break;
        case 341:
            self->demo_flag_0x08 &= 0x7F;
            eft035_throw_start(self, 0);
            break;
        case 342:
            if (Pl_frame_check(plw, 1, 26.0f, 0.0f) != 0) {
                copyVec3(&fx, &work->target_0x18);
                fx.y = 5.0f + work->floor_0x48;
                EFT_GROUND_BURST(2, &fx, 0.6f, 0, plw->area_0x16, 1);
                EFT_GROUND_BURST(1, &fx, 0.6f, 0, plw->area_0x16, 1);
                if (self->area_0x44 == get_now_areano()) {
                    se_req_orb(1, &fx);
                }
                self->demo_flag_0x08 &= 0x7F;
                eft035_throw_start(self, 1);
            }
            break;
        case 343:
            self->demo_flag_0x08 &= 0x7F;
            eft035_throw_start(self, 2);
            break;
        }
        eft035_orb_move(self);
        break;
    case 3:
        copyVec3(&wander, &work->vel_0x0C);
        if ((s32)work->scaled_0x40 == 0) {
            vec3_scale_in_place(&wander, 0.2f);
            rotVecY(&wander, (ran_suu(0) & 0x7FFF) - 0x4000);
        } else {
            vec3_scale_in_place(&wander, 0.5f);
            rotVecY(&wander, (ran_suu(0) & 0x3FFF) - 0x2000);
        }
        addVec3To(&work->vel_0x0C, &wander);
        if (vec3_len((f32*)&work->vel_0x0C) > 400.0f) {
            vec3_normalize_into(&work->vel_0x0C, &work->vel_0x0C);
            vec3_scale_in_place(&work->vel_0x0C, 400.0f);
        }
        if (self->pos_0x18.y >= work->target_0x18.y) {
            self->pos_0x18.y = work->target_0x18.y;
            work->vel_0x0C.y *= 0.2f;
            work->gravity_0x38 = 2.0f;
        } else {
            work->gravity_0x38 = 1.0f;
        }
        if ((self->timer_0x0C & 7) == 0) {
            copyVec3(&fx, &self->pos_0x18);
            fx.y = work->target_0x18.y;
            EFT_GROUND_BURST(2, &fx, 1.2f, 0, plw->area_0x16, 1);
            EFT_GROUND_BURST(1, &fx, 1.2f, 0, plw->area_0x16, 1);
            if (self->area_0x44 == get_now_areano()) {
                se_slot_req_pos(0, 37, &fx);
            }
        }
        eft035_orb_move(self);
        dist = vec3_distance(&self->pos_0x18, &work->target_0x18);
        if (dist > 100.0f) {
            copyVec3(&wander, &VEC3_SUB(&self->pos_0x18, &work->target_0x18));
            vec3_scale_in_place(&wander, 100.0f / dist);
            copyVec3(&self->pos_0x18, &VEC3_ADD(&work->target_0x18, &wander));
        }
        switch (Get_motion_no(plw)) {
        case 340:
            break;
        case 341:
            if (Pl_frame_check(plw, 1, 16.0f, 0.0f) != 0) {
                work->phase_0x41 = 0;
                copyVec3(&fx, &self->pos_0x18);
                fx.y = 5.0f + work->floor_0x48;
                eft035_throw_start(self, 0);
                EFT_GROUND_BURST(2, &fx, 2.5f, 0, plw->area_0x16, 1);
                EFT_GROUND_BURST(13, &fx, 1.4f, 0, plw->area_0x16, 1);
                if (self->area_0x44 == get_now_areano()) {
                    se_req_orb(2, &fx);
                }
            }
            break;
        default:
            self->state_0x05++;
            return;
        }
        break;
    case 4:
        motion = Get_motion_no(plw);
        if ((work->phase_0x41 == 0 && motion == 341) || (work->phase_0x41 == 1 && motion == 342) ||
            (work->phase_0x41 == 2 && motion == 343)) {
            work->timer_0x30++;
            switch (work->phase_0x41) {
            case 0:
                frames = 35;
                target_scale = 0.3f;
                break;
            case 1:
                frames = 27;
                target_scale = 0.5f;
                break;
            default:
                frames = 40;
                target_scale = 0.5f;
                break;
            }
            if ((s32)self->type_0x02 != 1) {
                if (work->scale_0x3C > target_scale) {
                    work->scale_0x3C -= (1.0f - target_scale) / frames;
                    if (work->scale_0x3C < target_scale) {
                        work->scale_0x3C = target_scale;
                    }
                }
            } else {
                target_scale = 0.3f;
            }
            if (work->phase_0x41 <= 1) {
                if (work->timer_0x30 >= frames) {
                    self->field_0x06++;
                    ((_EFT035_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
                    self->pos_0x18.y -= 10.0f;
                    work->timer_0x30 = 0;
                    work->vel_0x0C.x = 0.0f;
                    work->vel_0x0C.y = 0.0f;
                    work->vel_0x0C.z = 0.0f;
                    work->gravity_0x38 = 0.0f;
                    work->scale_0x3C = target_scale;
                }
            } else if (work->timer_0x30 < frames) {
                fx.x = work->target_0x18.x - self->pos_0x18.x;
                fx.y = work->target_0x18.y - self->pos_0x18.y;
                fx.z = work->target_0x18.z - self->pos_0x18.z;
                work->vel_0x0C.x = 0.09f * fx.x;
                work->vel_0x0C.y = 0.09f * fx.y;
                work->vel_0x0C.z = 0.09f * fx.z;
            } else {
                self->state_0x05++;
                return;
            }
            eft035_orb_move(self);
            break;
        }
        self->state_0x05++;
        return;
    case 5:
        motion = Get_motion_no(plw);
        if ((work->phase_0x41 == 0 && motion == 341) || (work->phase_0x41 == 1 && motion == 342)) {
            frames = (s32)work->phase_0x41 == 0 ? 50 : 20;
            if (++work->timer_0x30 < frames) {
                ((_EFT035_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
                self->pos_0x18.y -= 10.0f;
            } else {
                self->state_0x05++;
            }
            break;
        }
        self->state_0x05++;
        return;
    }
    work->stage_0x43 = ((_EFT035_PL_CHARGE*)plw)->stage_0x597;
    if (self->demo_flag_0x08 & 0x80) {
        if (eft035_throw_act_ck(self) == 1) {
            need = (Pl_Skill_ck((_PLW*)self->source_0x30, 149) == 1) + 2;
            if (((_EFT035_PL_CHARGE*)plw)->stage_0x597 >= (u8)need && work->vel_0x0C.y > 0.0f) {
                work->vel_0x0C.y = 0.0f;
            }
        } else if (work->vel_0x0C.y > 0.0f) {
            work->vel_0x0C.y = 0.0f;
        }
        col.r = 95;
        col.g = 95;
        col.b = 95;
        col.a = 255;
    } else {
        c = player_color_get(plw->chunk_ofs);
        col.r = c >> 24;
        col.g = (c >> 16) & 0xFF;
        col.b = (c >> 8) & 0xFF;
        col.a = c;
    }
    for (i = 0; i < 2; i++) {
        work->models_0x04[0]->setTevKColor(i, (_GXTevKColorID)3, &col);
    }
    copyVec3(&work->models_0x04[0]->pos_0x04, &self->pos_0x18);
    work->models_0x04[0]->field_0x2C = self->rot_0x24.y;
    eft035_ray_flicker(self);
    work->models_0x04[0]->move(0);
    eft_res_models_spawn(self, (void**)&work->models_0x04[0], 2, 1, 0);
}

/* Advances the tag-34 record's state index. */
extern "C" void fn_802F49C8(_EFT* self)
{
    self->state_0x05++;
}

/* Retires the tag-34 record once the effect is over. */
extern "C" void fn_802F49D8(_EFT* self)
{
    eft_res_slot_release(self);
}

/* While the orb hovers (phase 3) it flickers its ray at random for four frames: the ray stretches from the player's
 * weapon tip to the orb. */
extern "C" void eft035_ray_flicker(_EFT* self)
{
    Eft034Work* work = (Eft034Work*)self->work_0x38;
    nw4r::math::VEC3 tip;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 unused;
    nw4r::math::MTX34 mtx;
    f32 len;
    u32 ang_x;
    u32 ang_y;

    VEC3_ctor(&tip);
    VEC3_ctor(&dir);
    VEC3_ctor(&unused);
    MTX34_ctor(&mtx);
    if ((s32)self->field_0x06 == 3) {
        if ((s32)work->ray_on_0x42 == 0) {
            if ((ran_suu(0) & 7) == 0) {
                work->ray_on_0x42 = 1;
                work->ray_timer_0x34 = 0;
            }
        } else if (++work->ray_timer_0x34 >= 4) {
            work->ray_on_0x42 = 0;
        }
    } else {
        work->ray_on_0x42 = 0;
    }
    if (work->ray_on_0x42 == 1) {
        eft035_ray_origin_get(self, &tip);
        copyVec3(&dir, &VEC3_SUB(&self->pos_0x18, &tip));
        len = vec3_len((f32*)&dir);
        mtx34_identity(&mtx);
        calcVecAngXY(&dir, &ang_x, &ang_y);
        rotMatrixX(ang_x + 0xC000, &mtx);
        rotMatrixY(ang_y, &mtx);
        mtx34_set_trans(&mtx, &tip);
        work->models_0x04[1]->scale_0x1C.y = len;
        work->models_0x04[1]->move2(&mtx, 0);
        eft_res_models_spawn(self, (void**)&work->models_0x04[1], 2, 1, 0);
    }
}

/* Writes the ray's origin: the player's weapon joint 11 moved by the swing's tip offset (one of three, by the attack
 * motion and its frame). */
extern "C" void eft035_ray_origin_get(_EFT* self, nw4r::math::VEC3* out)
{
    _PLW* plw = (_PLW*)self->source_0x30;
    nw4r::math::MTX34 joint;
    nw4r::math::MTX34 rot;
    nw4r::math::VEC3 base;
    nw4r::math::VEC3 tip;
    s32 pose;

    MTX34_ctor(&joint);
    MTX34_ctor(&rot);
    VEC3_ctor(&base);
    VEC3_ctor(&tip);
    switch (Get_motion_no(plw)) {
    case 0x154:
        if (Pl_frame_check(plw, 2, 20.0f, 0.0f) != 0) {
            pose = 0;
        } else if (Pl_frame_check(plw, 2, 30.0f, 0.0f) != 0) {
            pose = 1;
        } else {
            pose = 2;
        }
        break;
    case 0x155:
        if (Pl_frame_check(plw, 2, 20.0f, 0.0f) != 0) {
            pose = 1;
        } else if (Pl_frame_check(plw, 2, 26.0f, 0.0f) != 0) {
            pose = 2;
        } else if (Pl_frame_check(plw, 2, 28.0f, 0.0f) != 0) {
            pose = 1;
        } else {
            pose = 0;
        }
        break;
    case 0x156:
        if (Pl_frame_check(plw, 2, 36.0f, 0.0f) != 0) {
            pose = 1;
        } else if (Pl_frame_check(plw, 2, 40.0f, 0.0f) != 0) {
            pose = 2;
        } else if (Pl_frame_check(plw, 2, 42.0f, 0.0f) != 0) {
            pose = 1;
        } else {
            pose = 0;
        }
        break;
    case 0x157:
        if (Pl_frame_check(plw, 2, 20.0f, 0.0f) != 0) {
            pose = 1;
        } else if (Pl_frame_check(plw, 1, 22.0f, 0.0f) != 0) {
            pose = 2;
        } else {
            pose = 0;
        }
        break;
    default:
        pose = 0;
        break;
    }
    mhchar_joint_mtx_get(&((_EFT035_PL_PHYS*)plw->physics_0x13C)->chr, 11, (Mtx34*)&joint);
    base.x = -9.2f;
    base.y = -1.7f;
    base.z = 12.0f;
    mulVecMat(&base, &joint);
    mtx34_identity(&rot);
    rotMatrixZ(0xC001, &rot);
    mtx34_concat_assign(&joint, &rot);
    tip.x = eft035_ray_tip_offsets[pose].x;
    tip.y = eft035_ray_tip_offsets[pose].y;
    tip.z = eft035_ray_tip_offsets[pose].z;
    mulVecMat(&tip, &joint);
    out->x = tip.x + (joint.m[0][3] + base.x);
    out->y = tip.y + (joint.m[1][3] + base.y);
    out->z = tip.z + (joint.m[2][3] + base.z);
}

/* Starts the orb's throw `phase`: the target sits in front of the player, the velocity carries it there over 35 (phase
 * 0) or 27 frames (phase 1) against the gravity; phase 2 stops it. */
extern "C" void eft035_throw_start(_EFT* self, u8 phase)
{
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 from;
    nw4r::math::VEC3 to;
    _PLW* plw;
    Eft034Work* work;
    f32 dist;

    VEC3_ctor(&ofs);
    VEC3_ctor(&from);
    VEC3_ctor(&to);
    plw = (_PLW*)self->source_0x30;
    work = (Eft034Work*)self->work_0x38;
    work->phase_0x41 = phase;
    ofs.x = 35.0f;
    ofs.y = 140.0f;
    ofs.z = 100.0f;
    rotVecY(&ofs, plw->field_0x058);
    copyVec3(&work->target_0x18, &VEC3_ADD(&plw->vec_0x03C, &ofs));
    self->field_0x06 = 4;
    from.x = self->pos_0x18.x;
    from.y = 0.0f;
    from.z = self->pos_0x18.z;
    to.x = work->target_0x18.x;
    to.y = 0.0f;
    to.z = work->target_0x18.z;
    switch (work->phase_0x41) {
    case 0:
        work->gravity_0x38 = -2.0f;
        copyVec3(&ofs, &VEC3_SUB(&to, &from));
        dist = vec3_len((f32*)&ofs);
        if (dist > 0.001f) {
            vec3_normalize_into(&ofs, &ofs);
            dist = dist / 35.0f;
            copyVec3(&work->vel_0x0C, &VEC3_SCALE(&ofs, dist));
            work->vel_0x0C.y = (work->target_0x18.y - self->pos_0x18.y) / 35.0f - 35.0f * (0.5f * work->gravity_0x38);
        } else {
            work->vel_0x0C.y = 0.0f;
        }
        break;
    case 1:
        work->gravity_0x38 = -1.0f;
        copyVec3(&ofs, &VEC3_SUB(&to, &from));
        dist = vec3_len((f32*)&ofs);
        if (dist > 0.001f) {
            vec3_normalize_into(&ofs, &ofs);
            dist = dist / 27.0f;
            copyVec3(&work->vel_0x0C, &VEC3_SCALE(&ofs, dist));
            work->vel_0x0C.y = (work->target_0x18.y - self->pos_0x18.y) / 27.0f - 27.0f * (0.5f * work->gravity_0x38);
        } else {
            work->vel_0x0C.y = 0.0f;
        }
        break;
    default:
        setVector3(&work->vel_0x0C, 0.0f, 0.0f, 0.0f);
        work->gravity_0x38 = 0.0f;
        break;
    }
    work->timer_0x30 = 0;
}

/* Moves the orb by its velocity and lets the gravity act on the velocity. */
extern "C" void eft035_orb_move(_EFT* self)
{
    Eft034Work* work = (Eft034Work*)self->work_0x38;

    addVec3To(&self->pos_0x18, &work->vel_0x0C);
    work->vel_0x0C.y += work->gravity_0x38;
}

/* Whether the player's act is one of the orb throws (0x15F, 0x161, 0x163, 0x185, 0x186). */
extern "C" u32 eft035_throw_act_ck(_EFT* self)
{
    switch (((_PLW*)self->source_0x30)->field_0x306) {
    case 0x15F:
    case 0x163:
    case 0x161:
    case 0x185:
    case 0x186:
        return TRUE;
    }
    return FALSE;
}

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
    for (int i = 0; i < lbl_80792860[type]; i++) {
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
    ((Eft035Work*)eft->work_0x38)->scale_0x04 = lbl_8079A9C8 * scale;
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
    work->count_0x00 = lbl_80792860[type];
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

