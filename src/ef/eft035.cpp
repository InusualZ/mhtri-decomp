/* ef/eft035.cpp - the `eft035` effect family, `.text` 0x802F140C..0x802F5138 (39 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: the runtime dump resolves only the two
 * `eft035_*` names - `eft035_set` at 0x802F2238 and `eft035_set2` at 0x802F2394 - and every other
 * address in the range is the dump's placeholder `zz_XXXXXXXX_`).
 *
 * Registration (docs/plan.md 12).  Class 2 evidence named it: the runtime dump's own name at
 * 0x802F2238 is `eft035_set`, so the module is `ef` and the file is `eft035.cpp` - the scheme of its
 * neighbours `ef/eft001.cpp` ... `ef/eft029.cpp`.  Language: both dumped names are C++ manglings
 * (`eft035_set__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3P10_CP_VECTORUcf`), so the file is `.cpp` and
 * every definition whose map name is plain (`fn_XXXXXXXX`) is `extern "C"` so its emitted name stays
 * the map's stem and objdiff can pair it (playbook row 42).  Sections: `extab`
 * 0x80015424..0x80015514 and `extabindex` 0x80033C9C..0x80033E04 (30 records each - exactly the
 * bytes the bracketing units leave unclaimed: `hud/fn_802EBED8.cpp` ends at 0x80015424/0x80033C9C
 * and the next unclaimed run starts at 0x80015514/0x80033E04).  No data section belongs to the unit
 * (the target object carries none); its `.data`/`.sdata`/`.sdata2` pool is `extern` here and never
 * defined (playbook 29).  One `.ctors` word belongs to the range, at 0x8056F38C - the address of the
 * file-scope static initialiser `fn_802F20D8`, read out of the DOL's `.ctors`; it is not claimed,
 * because dtk's auto split gave the range no `.ctors` unit.
 *
 * Seam (unproven).  This is one maximal unclaimed run, registered whole.  Two clusters share it and
 * there is no call edge between them: 0x802F140C..0x802F2238 (11 functions) drives the cockpit HUD
 * records (`lbl_806BDCC8`/`lbl_806BDFF0`, the `draw_sprite`/`drawshape` helpers) and carries the
 * file-scope static initialiser `fn_802F20D8`, while 0x802F2238..0x802F5138 (28 functions) is the
 * `eft035` family proper.  A *second* effect family (tag 34; `fn_802F39DC` seeds `field_0x03 = 34`
 * where `eft035_set` seeds 35) sits in the same half, so the range may hold more than one original
 * TU.  The extent settles as the functions match; the seam did not have to be cut to register, so it
 * was not.
 *
 * What it is.  `eft035_set`/`eft035_set2` are the spawn entry points of enemy effect 35: each
 * rejects a foreign area, takes a 64-byte (resp. 56-byte) work block from `fn_800F8788`, stamps
 * `field_0x03 = 35`, `type_0x02`, the source `_ENEMY_WORK` at +0x30, the position, the two copied
 * rotation words and the area, then installs the two hooks that travel with the record -
 * `fn_802F24E0` (`release_0x40`, the pool release) and `fn_802F2640` (`dispatch_0x34`, the
 * `state_0x05` machine).  The two setters differ in their work block: the type-0/1/3 family pools
 * 20-byte model records at work+0x08 (`memset` of the 16-byte handle list at work+0x30), the
 * type-4..7 family pools 4-byte model handles at work+0x08 (`memset` of the 24-byte list at
 * work+0x20).  Both block sizes are pinned by the allocation (`fn_800F8788(64)` / `fn_800F8788(56)`
 * against the 0x40 / 0x38 the two layouts need).
 *
 * The state machine is `fn_802F2640`: `state_0x05` 0 -> the type switch
 * (`fn_802F26B4`/`fn_802F288C`/`fn_802F2988`), 1 -> `fn_802F2C78`, 2 -> `fn_802F3940`,
 * 3 -> `fn_802F3950`.  `fn_802F2C78` is the alive-state body of the same family
 * (`fn_802F2CB0`/`fn_802F31F0`/`fn_802F3358`), and `fn_802F3B0C` is the machine of the tag-34 family
 * (`fn_802F3B48`/`fn_802F3D94`/`fn_802F49C8`/`fn_802F49D8`).
 *
 * Data.  The `.data` tables, the `.sdata` property tables (`lbl_80792860`, `lbl_80792868`,
 * `lbl_80792870`, `lbl_80792878`) and the `.sdata2` constants are `extern`-declared by their map
 * names and never defined (playbook 29).
 *
 * Residuals.  17 of the range's 39 functions are reconstructed; 11 of them are byte-identical and
 * every one but `eft035_set` is at or above the 80 % bar (`python tools/units/recompile.py
 * ef/eft035.cpp --main . --measure <symbol>`, against this worktree's own split target object):
 *
 *   byte-identical  fn_802F288C, fn_802F250C, fn_802F25BC, fn_802F2C78, fn_802F3940, fn_802F3950,
 *                   fn_802F3954, fn_802F3AD0, fn_802F3B0C, fn_802F49C8, fn_802F49D8
 *   fn_802F26B4     99.11  ours 468 B against the target's 472 B: retail keeps one more instruction
 *                          in the type-1 arm (the `em015_denki_eft_se_req` + `fn_802F3954` +
 *                          `eft019_set_core` argument setup)
 *   fn_802F39DC     99.18  the register colouring of the two-model seeding loop
 *   eft035_set2     98.73  ours 328 B against 332 B: we hoist `lbl_80792860[type]` where retail
 *                          re-reads it with `lbzx` inside the loop
 *   fn_802F24E0     94.55  the `== 2` arm: retail is `cmpwi` where our `u32` operand makes MWCC emit
 *                          `cmplwi` (the `<= 1` arm above it needs the unsigned form, so one operand
 *                          type cannot spell both tests)
 *   fn_802F2640     92.97  the outer switch: case 0 written first gives retail's chain order
 *                          (`cmpwi 0,1,2,3`) but its nested block then lands before the three
 *                          tail-call bodies (71.90); case 0 written last gives the bodies retail's
 *                          order and the chain 1,2,3,0 (92.97).  Retail has chain 0,1,2,3 *and*
 *                          bodies 1,2,3,0 - its case-0 body is a `b` to a block emitted after the
 *                          whole switch, which no source order we tried reproduces.
 *   eft035_set      73.85  the seeding loop's colouring: retail holds the narrowed type in r25 and
 *                          the table base in r26 and iterates with r23/r24 (`_savegpr_23`), we
 *                          hoist `lbl_80792860[type]` into r26 and save one register fewer
 *                          (`_savegpr_24`); our object is exactly the target's 348 B.  Tried: the
 *                          bound through a `u8` local (73.85), through a local table pointer (73.22
 *                          - worse), `memset(..., sizeof(work->works_0x30))` and reloading the work
 *                          pointer for the final scale store (71.32 -> 73.85).
 *
 * The 22 functions still to write, in address order (size): fn_802F140C (0x2B4), fn_802F16C0 (0x284),
 * fn_802F1944 (0x1D8), fn_802F1B1C (0x1DC), fn_802F1CF8 (0x23C), fn_802F1F34 (0x1A4),
 * fn_802F20D8 (0x48), fn_802F2120 (0x58), fn_802F2178 (0x30), fn_802F21A8 (0x60), fn_802F2208 (0x30),
 * fn_802F2988 (0x2F0), fn_802F2CB0 (0x540), fn_802F31F0 (0x168), fn_802F3358 (0x5E8),
 * fn_802F3B48 (0x24C), fn_802F3D94 (0xC34), fn_802F49DC (0x17C), fn_802F4B58 (0x2F0),
 * fn_802F4E48 (0x270), fn_802F50B8 (0x44), fn_802F50FC (0x3C).  What they need beyond this pass: the
 * 11 HUD functions drive `lbl_806BDCC8`/`lbl_806BDFF0` (their types live in `hud/cockpit_quest.h` and
 * `hud/fn_802EBED8.h`) plus the `draw_sprite`/`drawshape` helpers, and `fn_802F20D8` needs the
 * `__construct_array` runtime helper (whose only declaration today sits in `sound/sound_work.h`);
 * the two tag-34 bodies (`fn_802F3B48`, `fn_802F3D94`) read the `_ENEMY_WORK` motion/sound block and
 * `fn_802F50FC` reads a `u16` at `_ENEMY_WORK`+0x306 that the shared header does not name yet.
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
#include "g3d/g3d_calcworld.h"         /* fn_80073F68 */
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
 * selects the colour), the model handle `fn_800F8914` pools and `fn_800F8A44` releases, and the
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
 * size: 0x40 (the size `fn_800F8788(64)` pools) */
struct Eft035Work {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ f32 scale_0x04;
    /* +0x08 */ Eft035Slot slots_0x08[2];
    /* +0x30 */ struct _g3d_work* works_0x30[2][2];
};

/* Work block of the type-4..7 family (the 56-byte block `eft035_set2` pools): a 4-byte-stride model
 * handle array at +0x08 whose length is `count_0x00`, the create's returns at +0x14 and the handle
 * list at +0x20 (one 8-byte pair per model). size: 0x38 (the size `fn_800F8788(56)` pools) */
struct Eft035Work2 {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ f32 scale_0x04;
    /* +0x08 */ MHchar* models_0x08[3];
    /* +0x14 */ void* created_0x14[3];
    /* +0x20 */ struct _g3d_work* works_0x20[3][2];
};

/* Work block of the tag-34 family `fn_802F39DC` seeds (the 76-byte block `fn_800F8788(76)` pools):
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

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* Retail keeps the unfused countdown forms (`subi` + `cmpwi`, not the peephole's `subic.`) in the
 * two release loops - measured: with the pass on `fn_802F250C` is 89.93 and `fn_802F25BC` 87.03. */
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

/* Builds the family's first model (type 0/1/3): pools one model record's handle, binds the created
 * model to the effect's position and rotation, seeds its three random scales, then runs the
 * per-type setup. */
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
    _EFT* eft = (_EFT*)fn_800F8788(76);
    if (eft == 0) {
        return;
    }
    eft->release_0x40 = fn_802F3AD0;
    Eft034Work* work = (Eft034Work*)eft->work_0x38;
    eft->type_0x02 = type;
    work->count_0x00 = 2;
    for (int i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)fn_800F8914();
        if (work->models_0x04[i] == 0) {
            fn_800F886C(eft);
            return;
        }
    }
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
    fn_800F9DF4(eft, 1, 0);
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
    fn_800F886C(self);
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
    fn_800F886C(self);
}

/* Spawns the eft035 effect for a type-0/1/3 variant: builds the record, seeds the pool length from
 * the per-type table, takes one model record per entry, and installs position, rotation and area. */
void eft035_set(_ENEMY_WORK* self, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area,
                f32 scale)
{
    if (area != (u8)get_now_areano()) {
        return;
    }
    _EFT* eft = (_EFT*)fn_800F8788(64);
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
        work->slots_0x08[i].model_0x0C = (MHchar*)fn_800F8914();
        if (work->slots_0x08[i].model_0x0C == 0) {
            fn_800F886C(eft);
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
    fn_800F9DF4(eft, 0, 0);
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
    _EFT* eft = (_EFT*)fn_800F8788(56);
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
        work->models_0x08[i] = (MHchar*)fn_800F8914();
        if (work->models_0x08[i] == 0) {
            fn_800F886C(eft);
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
    fn_800F9DF4(eft, 0, 0);
    work->scale_0x04 = scale;
}

