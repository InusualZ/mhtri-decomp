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
 * RESIDUALS. 11 rows unwritten: 0x802F2988-0x802F2C78 (`fn_802F2988`), 0x802F2CB0-0x802F3940 (`fn_802F2CB0`,
 *   `fn_802F31F0`, `fn_802F3358`), 0x802F3B48-0x802F49C8 (`fn_802F3B48`, `fn_802F3D94`: they read the `_ENEMY_WORK`
 *   motion/sound block), 0x802F49DC-0x802F5138 (`fn_802F49DC`, `fn_802F4B58`, `fn_802F4E48`, `fn_802F50B8`,
 *   `fn_802F50FC`: it reads a `u16` at `_ENEMY_WORK`+0x306 the shared header does not name).
 *   6 partial rows:
 *  - `eft035_set`: retail holds the narrowed type in r25 and the table base in r26 and iterates with r23/r24
 *    (`_savegpr_23`); ours hoists `lbl_80792860[type]` into r26 and saves one register fewer (same 348 B);
 *  - `eft035_set2`: ours hoists `lbl_80792860[type]` where retail re-reads it with `lbzx` in the loop;
 *  - `fn_802F24E0`: the `== 2` arm is `cmpwi` in retail, `cmplwi` in ours (the `<= 1` arm needs the `u32` operand);
 *  - `fn_802F2640`: case 0 is written last, so ours compares 1, 2, 3, 0 where retail compares 0, 1, 2, 3
 *    (docs/ef.md, "The case-0 nested dispatch");
 *  - `fn_802F26B4`: retail keeps one more instruction in the type-1 arm's argument setup;
 *  - `fn_802F39DC`: the two-model seeding loop's registers are coloured differently.
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x994 of 0x2F00), extab (0x48 of 0x98) and
 *   extabindex (0x6C of 0xE4) short of the claim and differing.
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

/* ---------------------------------------------------------------------------------------------------
 * the per-family work blocks
 * ------------------------------------------------------------------------------------------------- */

/* One pooled model record of the type-0/1/3 family: the colour step the update walks (`state_0x00`
 * selects the colour), the model handle `eft_res_model_get` pools and `fn_800F8A44` releases, and the
 * model record `res_eft_UV_model_create` returned.  The release and the create both index this array
 * with a stride of 0x14 off work+0x08. size: 0x14 */
struct Eft035Slot {
    /* +0x00 */ u8 state_0x00;       /* the colour step the update advances, 0..3 */
    /* +0x01 */ _GXColor color_0x01; /* the colour `state_0x00` selects */
    /* +0x05 */ u8 unused_0x05[0x07];
    /* +0x0C */ MHchar* model_0x0C;  /* the pooled handle: `fn_800F8A44` releases this word */
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

/* Work block of the tag-34 family `fn_802F39DC` seeds (the 76-byte block `eft_res_slot_get(76)` pools):
 * the pooled model handles at +0x04, `field_0x00` their used length (always 2) and the rest the
 * family's own body reads. size: 0x4C */
struct Eft034Work {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[2];
    /* +0x0C */ u8 unused_0x0C[0x4C - 0x0C];
};

/* ---------------------------------------------------------------------------------------------------
 * the unit's own `.sdata`/`.sdata2` pool, referenced by name and never defined (playbook 29)
 * ------------------------------------------------------------------------------------------------- */

extern "C" const u8 lbl_80792860[8]; /* per-type pool length, both eft035 families */
extern "C" const u8 lbl_80792868[8]; /* type -> variant index, both eft035 families */
extern "C" f32 lbl_8079A9C8;         /* the scale factor eft035_set multiplies its argument by */

#pragma peephole off

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
        fn_800F8A44(&work->slots_0x08[i].model_0x0C, 1);
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
    fn_800F8A44(&work->models_0x08[0], work->count_0x00);
    work->count_0x00 = 0;
}

/* The record's pool release: picks the family's release handler from the variant the type table
 * selects. */
extern "C" void fn_802F24E0(_EFT* self)
{
    u32 variant = lbl_80792868[self->type_0x02];

    switch (variant) {
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
extern "C" void fn_802F2640(_EFT* self)
{
    switch (self->state_0x05) {
    case 1:
        fn_802F2C78(self);
        break;
    case 2:
        fn_802F3940(self);
        break;
    case 3:
        fn_802F3950(self);
        break;
    case 0:
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

    void* model = res_eft_UV_model_create(work->slots_0x08[0].model_0x0C, 53, 280, 0,
                                          (struct _g3d_work**)&work->works_0x30[0][0], 1, 0);
    work->slots_0x08[0].created_0x10 = model;
    if (model == 0) {
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

/* Spawns the tag-34 effect: pools two model handles, seeds the family's zeroed position/rotation and
 * clock, then installs the two hooks that travel with the record. */
extern "C" void fn_802F39DC(_ENEMY_WORK* self, u8 type)
{
    _EFT* eft = (_EFT*)eft_res_slot_get(76);
    if (eft == 0) {
        return;
    }
    eft->release_0x40 = fn_802F3AD0;
    Eft034Work* work = (Eft034Work*)eft->work_0x38;
    eft->type_0x02 = type;
    work->count_0x00 = 2;
    for (int i = 0; i < work->count_0x00; i++) {
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

    fn_800F8A44(&work->models_0x04[0], work->count_0x00);
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
    work->scale_0x04 = scale;
}

