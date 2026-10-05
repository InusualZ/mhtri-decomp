/* ef/eft007.cpp - effect 007 (the player-weapon controller and the enemy emitter)
 *
 * `.text` 0x80102994..0x80103D28, 16 functions written (the rest of the range is not decompiled yet).
 * Phase 4: recut registered unit; the tail of the retired `ef/eft007.cpp` (the `em` head went to `ef/em_effect_ctrl.cpp`).
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2: the owner's header) */
#include "ef/mtx34_trans_get.h" /* mtx34_trans_get (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "Pl/plw.h"
#include "gx.h"
#include "ef/eft004.h"
#include "ef/eft009.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "ef/pRoot.h"
#include "ef/eft007_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_rot_vec_copy_c1 ((void (*)(_CP_VECTOR*, const _CP_VECTOR*))eft_rot_vec_copy)

/* --- callees and pool constants the whole unit shares ------------------------------------------- */

/* untyped: an opaque handle passed through (the effect record is only handed on) */
extern "C" void eft_res_models_spawn(void* self, void* list, u32 mode, s32 count, u32 arg);
/* untyped: an opaque handle passed through (the pooled slot is only handed on) */
extern "C" void eft_res_slot_release(void* self);

/* `eft_res_slot_get` hands out one of the 0x48-byte effect pool slots; the three families each view it as
 * their own record, so the shared prototype returns `void*` and each call site names its view. */
extern "C" void* eft_res_slot_get(u32 pool_id);
/* untyped: an opaque handle passed through (the callee only hands the pointer on) */
extern "C" void eft_state_flags_set(void* self, u8 a, u8 b);
struct _CP_VECTOR;

/* get_now_areano / get_now_mapno / eftGetKeyAlpha / vec_to_mh_vec3 come from `unsplit/unknown.h` as
 * their real declarations (rule 9).  get_camera_pos / get_camera_direction are NOT converted: their map
 * names are no-argument manglings (`__Fv`) but every call site passes an out pointer, so the only
 * declaration that reproduces the target's codegen is the `extern "C"` map spelling - the real
 * `nw4r::math::VEC3 get_camera_pos()` (struct return, sret) grows the frame 0x1C0 -> 0x1F0 and drops the
 * unit's score, so it is left and reported (rule 9, no real name expressible without a score change).
 * move/setTevKColor are `MHchar` members whose owner (the canonical `pl.h` struct) does not carry them
 * yet - left as the map spelling and reported. */
#include "camera/camera.h" /* get_camera_pos (rule 2: the owner's header) */
/* The call keeps the out-pointer view (`void (VEC3*)`) the target's frame needs: a cast call to the owner's by-value `get_camera_pos` is the same direct call to the map symbol. */
#define get_camera_pos_c1 ((void (*)(nw4r::math::VEC3*))get_camera_pos)

extern f32 lbl_80796740; /* 0.0f     */

namespace nw4r {

namespace ef {
/* The emitter object the pool block holds. Only ever used through pointers here, so the size is the
 * smallest an empty class can be - a lower bound, an approximation. */
struct Effect { /* size: 0x04 - lower bound, an approximation (opaque here) */
    void SetRootMtx(const nw4r::math::MTX34& mtx);
    void RetireEmitterAll();
};
}
}

/* `_CP_VECTOR` (the three-word rotation vector the setters copy in; x/y are the joint ids `rotLocalMatX/Y`
 * take) comes from `ef/cp_vector.h` - one definition, in the owner's header (rule 1). */

struct _MHcharJoints;

/* The character/model an effect hangs off (`MHchar` in the map's mangling); this block only calls its
 * joint-position member.  size: 0x140 - lower bound, an approximation (Pl/pl_act.cpp's extent) */
struct MHchar {
    /* +0x000 */ u8 unused_0x000[0x140];

    void get_joint_wpos(unsigned long joint, nw4r::math::VEC3* out);
};

/* What `_PLW::physics_0x13C` points at: a 4-byte word and then the joint-bearing character the joint
 * calls are made on. Only ever used through the pointer. size: 0x144 - lower bound, an approximation. */
struct _MHcharJoints {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ MHchar joint_0x004;
};

/* `_PLW`, the player work the two setters take, comes from `Pl/plw.h` - one definition, in the owner's header
 * (rule 1). */
/* The effect object `eft_res_slot_get(44)` returns and the two setters fill: the pool block at +0x38, the two
 * handlers at +0x34/+0x40 and the source character at +0x30. */
struct _EFT007;

/* The effect's pool block: a count, the two pooled emitters it covers, the parameter scale/id and the two
 * vectors the per-frame bodies place it by. size: 0x2C */
struct _EFT007_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ f32 paramscale;
    /* +0x10 */ u32 param_id;
    /* +0x14 */ nw4r::math::VEC3 pos_0x14;
    /* +0x20 */ nw4r::math::VEC3 joint_pos_0x20;
};

struct _EFT007 { /* size: 0x48 */
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 step_0x06;
    /* +0x07 */ u8 rot_flag_0x07;
    /* +0x08 */ u8 colour_index_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 delay_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ _CP_VECTOR rot_0x24;
    /* +0x30 */ _PLW* model_0x30;
    /* +0x34 */ void (*dispatch_0x34)(_EFT007* self);
    /* +0x38 */ _EFT007_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(_EFT007* self);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* ---------------------------------------------------------------------------------------------------
 * the callees this block calls. A plain name is what `symbols.txt` spells, so it is `extern "C"`; a
 * mangled one is a C++ declaration whose signature reproduces the map's argument list.
 * ------------------------------------------------------------------------------------------------- */
extern "C" s32 eft_res_spawn_gate_ck(void* self, u32 arg);
extern "C" void fn_800F996C(nw4r::ef::Effect* effect, u32 arg);
extern "C" void fn_800FBB90(nw4r::math::MTX34* mtx, nw4r::math::VEC3* vec);
extern "C" u8 fn_803311A0(MHchar* model);
extern "C" u8 fn_80331210(_PLW* self);

void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
void setVector3(nw4r::math::VEC3* out, f32 x, f32 y, f32 z);
nw4r::ef::Effect* res_eft_create(u16 id, u16 kind, unsigned long arg);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
u32 effect_move(nw4r::ef::Effect* effect);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
void mulVecMat(nw4r::math::VEC3* vec, nw4r::math::MTX34* mtx);
void rotLocalMatX(unsigned long joint, nw4r::math::MTX34* mtx);
void rotLocalMatY(unsigned long joint, nw4r::math::MTX34* mtx);
void rotLocalMatZ(unsigned long joint, nw4r::math::MTX34* mtx);
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
u32 get_stg_eft_col(u8 area, u8 kind);
u32 Pl_frame_check(_PLW* plw, unsigned long frame, f32 a, f32 b);

/* --- this block's own functions, so the setters can name their two handlers before their bodies --- */
extern "C" void fn_80102CD0(_EFT007* self);
extern "C" void fn_80102D0C(_EFT007* self);
extern "C" void fn_80102D48(_EFT007* self);
extern "C" void fn_80103130(_EFT007* self);
extern "C" void fn_80103518(_EFT007* self);
extern "C" void fn_801036BC(_EFT007* self);
extern "C" s32 fn_801036C0(_EFT007* self);
extern "C" void fn_8010383C(_EFT007* self, nw4r::math::MTX34* mtx);

/* --- the unit's own `.data`/`.sdata2` pool, referenced but never emitted here (playbook 29) --- */
extern "C" u16 lbl_8059DAA0[];
extern "C" u16 lbl_8059DAC8[];
extern "C" u8 lbl_8059DAF0[];
extern f32 lbl_80796744;
extern f32 lbl_80796748;
extern f32 lbl_8079674C;

namespace nw4r {

namespace ef {
struct Effect;
}
}

/* The engine's 3-word position/rotation triple that `eft_rot_vec_copy` copies (`Pl/pl_act.cpp` spells it
 * the same way). */
/* The enemy the emitter hangs off.  Only the bytes this block reads are named: the joint position the
 * emitter copies out of (its y component is added to the caller's height), the area number
 * `get_now_areano()` is compared against, and the joint-variant flags that pick the effect subtype. */
struct _ENEMY_WORK {
    /* +0x000 */ u8 unused_0x000[0x1BC];
    /* +0x1BC */ _CP_VECTOR pos_0x1BC;
    /* +0x1C8 */ u8 unused_0x1C8[0x1E1 - 0x1C8];
    /* +0x1E1 */ u8 area_no;
    /* +0x1E2 */ u8 unused_0x1E2[0x228 - 0x1E2];
    /* +0x228 */ u16 joint_flags; /* bit 1 selects the `0x25` subtype over `0x27` */
    /* +0x22A */ u8 unused_0x22A[0x22C - 0x22A];
};
/* size: 0x22C - lower bound (the enemy record continues past what this block reads). */
struct _EFT_EMITTER;
struct _EFT_JOINT;

typedef void (*EftEmitterHook)(_EFT_EMITTER* self);

/* The work block at `_EFT_EMITTER::joint`: the joint's effect list plus the world matrix and the
 * offset the state machines transform through.  `get_joint_wmat_em` fills the matrix at `+0x0C` and
 * `vec_to_mh_vec3` the offset at `+0x3C`, which is what fixes the stride between them; the count is
 * `lbl_8059DE00[part]`, i.e. 1 or 2. */
struct _EFT_JOINT {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ nw4r::math::MTX34 mtx;
    /* +0x3C */ nw4r::math::VEC3 offset;
};
/* size: 0x48 - lower bound (the block `fn_800F8B44` hands back is larger). */

/* The 0x48-byte emitter `eft_res_slot_get(0x48)` hands out. */
struct _EFT_EMITTER {
    /* +0x00 */ u8 in_use; /* set to 1 by the pool allocator */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 type; /* the joint/motion id, indexes lbl_8059DCF8 */
    /* +0x03 */ u8 eft_id; /* 8 for this family */
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 frame; /* the step fn_80103CEC dispatches on */
    /* +0x06 */ u8 unused_0x06;
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ u8 unused_0x08[0x0C - 0x08];
    /* +0x0C */ s32 timer_0x0C; /* the countdown fn_801041BC decrements */
    /* +0x10 */ s32 unused_0x10; /* zeroed by fn_80103B60, never read by this family */
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ _CP_VECTOR rot; /* the joint rotation mtx34_trans_get reads */
    /* +0x24 */ _CP_VECTOR pos; /* the joint position, copied out of the enemy */
    /* +0x30 */ _ENEMY_WORK* owner;
    /* +0x34 */ EftEmitterHook advance; /* fn_80103CEC */
    /* +0x38 */ _EFT_JOINT* joint;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ EftEmitterHook retire; /* fn_80103CB0 */
    /* +0x44 */ u8 area_no;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};
/* size: 0x48 */

/* Callees: a plain name is what `symbols.txt` spells, so it is declared `extern "C"`; the three
 * mangled ones are declared as C++ functions so the mangling reproduces the map's symbol. */
extern "C" void fn_80103CB0(_EFT_EMITTER* self);
extern "C" void fn_80103CEC(_EFT_EMITTER* self);
/* fn_80103D28 / fn_801041BC / fn_801048A0 / fn_801048B0 come from their owner's header (rule 2). */
extern "C" _EFT_EMITTER* fn_80103B60(_ENEMY_WORK* enemy, u8 part);

void vec_to_mh_vec3(nw4r::math::VEC3* dst, Vec* src);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* mtx);

/* The block's pooled data - declared, never defined here (the pools belong to the data pass). */
#include "unsplit/ef_tables.h" /* lbl_8059DCF8 / lbl_8059DE00 / lbl_8059DE48 (rule 2: the band) */

#pragma peephole off

/* Spawns the `eft007` effect for the player's weapon: builds the object, seeds its pool block with the
 * id, the scale and the placement vector, maps the weapon type onto the effect's type/colour pair and
 * installs the two handlers. */
void eft007_set(_PLW* self, u8 type, u8 colour, unsigned long id, nw4r::math::VEC3* vec, f32 scale)
{
    if (self->area_0x16 != (u8)get_now_areano()) {
        return;
    }
    _EFT007* effect = (_EFT007*)eft_res_slot_get(44);
    if (effect == 0) {
        return;
    }
    _EFT007_WORK* work = effect->work_0x38;
    work->count = 1;
    work->param_id = id;
    work->paramscale = scale;
    if (vec != 0) {
        copyVec3(&work->pos_0x14, vec);
    } else {
        setVector3(&work->pos_0x14, lbl_80796740, lbl_80796740, lbl_80796740);
    }
    u8 motion = fn_80331210(self);
    switch (type) {
    case 3:
    case 4:
    case 6:
    case 9:
    case 16:
        switch (motion) {
        case 0:
        case 1:
        case 2:
            colour = motion;
            break;
        case 3:
            if (type != 3) {
                type += 1;
            } else {
                colour = motion;
            }
            break;
        }
        break;
    }
    effect->model_0x30 = self;
    effect->type_0x02 = type;
    effect->field_0x03 = 2;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    effect->area_0x44 = self->area_0x16;
    effect->colour_index_0x08 = colour;
    eft_state_flags_set(effect, 0, 0);
    effect->release_0x40 = fn_80102CD0;
    effect->dispatch_0x34 = fn_80102D0C;
}

/* The same spawn for a caller-supplied rotation vector: the vector is copied into the effect's rotation
 * slot and the rotation flag is raised, so the per-frame body applies the two local rotations. */
void eft007_set_vec(_PLW* self, u8 type, u8 colour, unsigned long id, nw4r::math::VEC3* vec, f32 scale,
                    _CP_VECTOR* rot)
{
    if (self->area_0x16 != (u8)get_now_areano()) {
        return;
    }
    _EFT007* effect = (_EFT007*)eft_res_slot_get(44);
    if (effect == 0) {
        return;
    }
    _EFT007_WORK* work = effect->work_0x38;
    work->count = 1;
    work->param_id = id;
    work->paramscale = scale;
    if (vec != 0) {
        copyVec3(&work->pos_0x14, vec);
    } else {
        setVector3(&work->pos_0x14, lbl_80796740, lbl_80796740, lbl_80796740);
    }
    u8 motion = fn_80331210(self);
    switch (type) {
    case 3:
    case 4:
    case 6:
    case 9:
    case 16:
        switch (motion) {
        case 0:
        case 1:
        case 2:
            colour = motion;
            break;
        case 3:
            if (type != 3) {
                type += 1;
            } else {
                colour = motion;
            }
            break;
        }
        break;
    }
    effect->model_0x30 = self;
    effect->type_0x02 = type;
    effect->field_0x03 = 2;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    eft_rot_vec_copy_c1(&effect->rot_0x24, rot);
    effect->area_0x44 = self->area_0x16;
    effect->colour_index_0x08 = colour;
    effect->rot_flag_0x07 = 1;
    eft_state_flags_set(effect, 0, 0);
    effect->release_0x40 = fn_80102CD0;
    effect->dispatch_0x34 = fn_80102D0C;
}

/* Releases the effect's pool block: hands every pooled emitter back and clears the count. */
extern "C" void fn_80102CD0(_EFT007* self)
{
    _EFT007_WORK* work = self->work_0x38;
    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* Runs the effect's `state_0x05` handler - the per-frame step of the animation. */
extern "C" void fn_80102D0C(_EFT007* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80102D48(self);
        return;
    case 1:
        fn_80103130(self);
        return;
    case 2:
        fn_80103518(self);
        return;
    case 3:
        fn_801036BC(self);
        return;
    }
}

/* Places and advances the first step of the animation family: builds the two pooled emitters from the
 * weapon type's resource pair and runs the per-type placement. Every arm shares one tail (the model's
 * joint position then the second step), so the arms only `break`. */
extern "C" void fn_80102D48(_EFT007* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    _PLW* model = self->model_0x30;
    _EFT007_WORK* work = self->work_0x38;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    self->state_0x05++;
    work->effects[0] =
        res_eft_create(lbl_8059DAA0[self->type_0x02], lbl_8059DAC8[self->type_0x02], 0);
    if (work->effects[0] == 0) {
        fn_801036BC(self);
        return;
    }
    switch (self->type_0x02) {
    case 0:
    case 3:
    case 7:
    case 8:
    case 11:
        break;
    case 1:
    case 2:
    case 12:
    case 13:
    case 14:
    case 15: {
        copyVec3(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        if (self->rot_flag_0x07 == 1) {
            rotLocalMatY(self->rot_0x24.y, &mtx);
            rotLocalMatX(self->rot_0x24.x, &mtx);
        }
        for (s32 i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            change_paramscale_eff(work->effects[i], work->paramscale);
        }
        break;
    }
    case 4:
        self->timer_0x0C = 7;
        work->effects[1] = res_eft_create(0x632, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_801036BC(self);
            return;
        }
        work->count++;
        break;
    case 5:
        self->timer_0x0C = 7;
        break;
    case 6:
        work->effects[1] = res_eft_create(0x632, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_801036BC(self);
            return;
        }
        work->count++;
        break;
    case 9:
        work->effects[1] = res_eft_create(0x6C5, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_801036BC(self);
            return;
        }
        work->count++;
        if (fn_803311A0((MHchar*)model) >= 2) {
            self->timer_0x0C = 3;
            work->paramscale *= lbl_80796744;
        } else {
            self->timer_0x0C = 12;
        }
        copyVec3(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        fn_800DD7E0((MHchar*)model, &self->pos_0x18, 0);
        break;
    case 10:
        if (fn_803311A0((MHchar*)model) >= 2) {
            self->timer_0x0C = 3;
            work->paramscale *= lbl_80796744;
        } else {
            self->timer_0x0C = 12;
        }
        copyVec3(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        fn_800DD7E0((MHchar*)model, &self->pos_0x18, 0);
        break;
    case 16:
    case 17:
        self->timer_0x0C = 0x14;
        break;
    case 18:
        fn_8010383C(self, &mtx);
        copyVec3(&pos, &work->pos_0x14);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        change_paramscale_eff(work->effects[0], work->paramscale);
        break;
    case 19:
        fn_8010383C(self, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        change_paramscale_eff(work->effects[0], work->paramscale);
        break;
    }
    ((_MHcharJoints*)model->physics_0x13C)->joint_0x004.get_joint_wpos(work->param_id, &work->joint_pos_0x20);
    fn_80103130(self);
}

/* Per-frame body of the second step of the animation family: re-places and drives every pooled emitter,
 * recolours them from the weapon type's key table (or the stage's colour), then either hands them back to
 * the pool or, for three of the types, waits for the camera to move past the model. */
extern "C" void fn_80103130(_EFT007* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;
    /* Retail initialises four vectors here; only `pos` is read back, but the other three
     * `VEC3_ctor` calls print in the object, so the locals have to stay. */
    nw4r::math::VEC3 spare_a;
    nw4r::math::VEC3 spare_b;
    nw4r::math::VEC3 spare_c;
    nw4r::math::VEC3 cam;
    _GXColor color;
    _EFT007_WORK* work = self->work_0x38;
    _PLW* model = self->model_0x30;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    VEC3_ctor(&spare_a);
    VEC3_ctor(&spare_b);
    VEC3_ctor(&spare_c);
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (((u32)(self->type_0x02 - 3) <= 2 || (u32)(self->type_0x02 - 9) <= 1 ||
         (u32)(self->type_0x02 - 16) <= 1) &&
        model->field_0x00A != 4) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        self->step_0x06 = 2;
        return;
    }
    s32 timer = --self->timer_0x0C;
    u32 do_place;
    switch (self->type_0x02) {
    case 1:
    case 2:
    case 12:
    case 13:
    case 14:
    case 15:
    case 18:
    case 19:
        do_place = 0;
        break;
    case 4:
    case 5:
    case 9:
    case 10:
    case 16:
    case 17:
        if (timer < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        do_place = 1;
        break;
    case 6:
    case 7:
        if (fn_801036C0(self) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            self->step_0x06 = 0;
            return;
        }
        do_place = 1;
        break;
    case 0:
    case 3:
    case 8:
    case 11:
    default:
        do_place = 1;
        break;
    }
    if (do_place == 1) {
        copyVec3(&pos, &work->pos_0x14);
        fn_8010383C(self, &mtx);
        mulVecMat(&pos, &mtx);
        mtx34_trans_add(&mtx, &pos);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        if (self->rot_flag_0x07 == 1) {
            rotLocalMatY(self->rot_0x24.y, &mtx);
            rotLocalMatX(self->rot_0x24.x, &mtx);
        }
        for (s32 i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            change_paramscale_eff(work->effects[i], work->paramscale);
        }
    }
    for (s32 i = 0; i < work->count; i++) {
        if (effect_move(work->effects[i]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    if ((u32)(self->type_0x02 - 13) > 2) {
        switch (self->type_0x02) {
        case 3:
            color.r = lbl_8059DAF0[self->colour_index_0x08 * 4];
            color.g = lbl_8059DAF0[self->colour_index_0x08 * 4 + 1];
            color.b = lbl_8059DAF0[self->colour_index_0x08 * 4 + 2];
            color.a = lbl_8059DAF0[self->colour_index_0x08 * 4 + 3];
            change_color_eff(work->effects[0], &self->pos_0x18, color);
            break;
        case 4:
        case 6:
        case 9:
            color.r = lbl_8059DAF0[self->colour_index_0x08 * 4];
            color.g = lbl_8059DAF0[self->colour_index_0x08 * 4 + 1];
            color.b = lbl_8059DAF0[self->colour_index_0x08 * 4 + 2];
            color.a = lbl_8059DAF0[self->colour_index_0x08 * 4 + 3];
            change_color_eff(work->effects[1], &self->pos_0x18, color);
            break;
        }
    } else {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        color.r = (col & 0xFF000000) >> 24;
        color.g = (col & 0x00FF0000) >> 16;
        color.b = (col & 0x0000FF00) >> 8;
        color.a = col & 0xFF;
        change_color_eff(work->effects[0], &self->pos_0x18, color);
    }
    if ((u32)(self->type_0x02 - 13) <= 2) {
        get_camera_pos_c1(&cam);
        if (cam.y < model->field_0x064) {
            eft_res_models_spawn(self, work->effects, 1, work->count, 0);
        }
    } else {
        eft_res_models_spawn(self, work->effects, 1, work->count, 0);
    }
}

/* Per-frame body of the third step: a two-stage retire/park state machine over the pooled emitters. */
extern "C" void fn_80103518(_EFT007* self)
{
    nw4r::math::VEC3 vec;
    nw4r::math::MTX34 mtx;
    _EFT007_WORK* work = self->work_0x38;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);

    if ((u32)(self->type_0x02 - 6) <= 1 || (u32)(self->type_0x02 - 9) <= 1 ||
        (u32)(self->type_0x02 - 16) <= 1) {
        switch (self->step_0x06) {
        case 0: {
            self->flag_0x01 = 1;
            self->step_0x06++;
            self->delay_0x10 = 10;
            for (s32 i = 0; i < work->count; i++) {
                work->effects[i]->RetireEmitterAll();
            }
            /* fallthrough */
        }
        case 1: {
            copyVec3(&vec, &work->pos_0x14);
            fn_8010383C(self, &mtx);
            mulVecMat(&vec, &mtx);
            mtx34_trans_add(&mtx, &vec);
            mtx34_trans_get(&mtx, &self->pos_0x18);
            for (s32 i = 0; i < work->count; i++) {
                work->effects[i]->SetRootMtx(mtx);
                fn_800F996C(work->effects[i], 0);
            }
            if (--self->delay_0x10 < 0) {
                self->state_0x05++;
                return;
            }
            eft_res_models_spawn(self, work->effects, 1, work->count, 0);
            return;
        }
        default:
            self->state_0x05++;
            break;
        }
    } else {
        self->state_0x05++;
    }
}

/* Tail-destroys an effect object (the emitter pools are released by its `release_0x40` handler first). */
extern "C" void fn_801036BC(_EFT007* self)
{
    eft_res_slot_release(self);
}

/* Answers whether the model is in one of the motion states that park or retire the effect: 1 when the
 * motion is one of the `Pl_frame_check` families, otherwise the step machine is walked to its end state
 * (which retires the pooled emitters once) and 0 comes back. */
extern "C" s32 fn_801036C0(_EFT007* self)
{
    _PLW* model = self->model_0x30;

    if (model->field_0x00A == 4) {
        switch (model->act_no) {
        case 9:
        case 22:
        case 53:
        case 54:
        case 72:
        case 86:
        case 87:
        case 88:
            return Pl_frame_check(model, 2, lbl_80796748, lbl_80796740) == 1;
        case 24:
        case 55:
            if (self->step_0x06 == 0) {
                self->step_0x06++;
                fn_800DD7E0((MHchar*)model, &self->pos_0x18, 1);
            }
            return 1;
        case 10:
        case 56:
            if (Pl_frame_check(model, 2, lbl_8079674C, lbl_80796740) == 1) {
                return 1;
            }
            if (self->step_0x06 == 1) {
                self->step_0x06 = 2;
                fn_800DD7E0((MHchar*)model, &self->pos_0x18, -1);
            }
            return 0;
        }
    }
    if (self->step_0x06 == 1) {
        self->step_0x06 = 2;
        fn_800DD7E0((MHchar*)model, &self->pos_0x18, -1);
    }
    return 0;
}

/* Builds the model's placement matrix for the effect: reads the model's joint matrix, then applies the
 * per-effect-type local rotation set (which is what gives each weapon type its own swing axis). */
extern "C" void fn_8010383C(_EFT007* self, nw4r::math::MTX34* mtx)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 origin;
    _PLW* model = self->model_0x30;
    _EFT007_WORK* work = self->work_0x38;

    VEC3_ctor(&pos);
    VEC3_ctor(&origin);
    fn_800E0A14(&((_MHcharJoints*)model->physics_0x13C)->joint_0x004, work->param_id, mtx);
    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 8:
    case 13:
        rotLocalMatY(0xF82F, mtx);
        rotLocalMatX(0xF99B, mtx);
        rotLocalMatZ(0xFE95, mtx);
        return;
    case 6:
    case 7:
    case 15:
        rotLocalMatY(0xE4FB, mtx);
        rotLocalMatX(0xFFA6, mtx);
        rotLocalMatZ(0xECCE, mtx);
        return;
    case 9:
    case 10:
    case 11:
    case 12:
    case 14:
        rotLocalMatZ(0x8000, mtx);
        return;
    case 16:
    case 17:
    case 18:
        mtx34_trans_get(mtx, &pos);
        cpSetRotMatrix(&model->rot_0x54, mtx);
        fn_800FBB90(mtx, &pos);
        return;
    }
}

#pragma peephole on

/* Builds the emitter for the enemy's part and hands it back. */
extern "C" void* eft007_part_set(_ENEMY_WORK* enemy, u32 part)
{
    return fn_80103B60(enemy, part);
}

/* Builds the part's emitter and, when it exists, copies the emitter's joint offset into the caller's
 * vector. */
extern "C" void fn_80103968(_ENEMY_WORK* enemy, u32 part, nw4r::math::VEC3* offset)
{
    _EFT_EMITTER* emitter = (_EFT_EMITTER*)fn_80103B60(enemy, part);

    if (emitter != NULL) {
        copyVec3(&emitter->joint->offset, offset);
    }
}

#pragma peephole off

extern "C" void eft007_part_spawn(_ENEMY_WORK* enemy, u8 part, s32 pos_x, s32 pos_y)
{
    if (enemy->area_no == get_now_areano()) {
        _EFT_EMITTER* emitter = (_EFT_EMITTER*)eft_res_slot_get(0x48);

        if (emitter != NULL) {
            _EFT_JOINT* joint = emitter->joint;

            joint->count = lbl_8059DE00[part & 0xFF];
            vec_to_mh_vec3(&joint->offset, &lbl_8059DE48[part & 0xFF]);
            emitter->eft_id = 8;
            emitter->type = part;

            if ((s32)emitter->type == 0x24) {
                switch (get_now_mapno()) {
                case 4:
                case 15:
                    emitter->type = 0x26;
                    break;
                case 5:
                case 16:
                    if (enemy->joint_flags & 6) {
                        emitter->type = 0x25;
                    } else {
                        emitter->type = 0x27;
                    }
                    break;
                default:
                    if (enemy->joint_flags & 6) {
                        emitter->type = 0x25;
                    }
                    break;
                }
            }

            emitter->owner = enemy;
            emitter->area_no = enemy->area_no;

            if (emitter->type == 2) {
                emitter->timer_0x0C = 4;
            } else {
                emitter->timer_0x0C = 0;
            }

            emitter->pos.x = pos_x;
            emitter->pos.y = pos_y + enemy->pos_0x1BC.y;
            emitter->pos.z = 0;

            eft_state_flags_set(emitter, 0, 0);
            get_joint_wmat_em(enemy, lbl_8059DCF8[emitter->type], &joint->mtx);

            emitter->retire = fn_80103CB0;
            emitter->advance = fn_80103CEC;
        }
    }
}

extern "C" _EFT_EMITTER* fn_80103B60(_ENEMY_WORK* enemy, u8 part)
{
    _EFT_EMITTER* emitter;
    _EFT_JOINT* joint;

    if (enemy->area_no != get_now_areano() || (u32)part == 7) {
        return NULL;
    }

    emitter = (_EFT_EMITTER*)eft_res_slot_get(0x48);
    if (emitter == NULL) {
        return NULL;
    }

    joint = emitter->joint;
    joint->count = lbl_8059DE00[part & 0xFF];
    vec_to_mh_vec3(&joint->offset, &lbl_8059DE48[part & 0xFF]);
    emitter->eft_id = 8;
    emitter->type = part;
    emitter->owner = enemy;
    emitter->area_no = enemy->area_no;

    if (emitter->type == 2) {
        emitter->timer_0x0C = 4;
    } else {
        emitter->timer_0x0C = 0;
    }

    emitter->unused_0x10 = 0;
    eft_rot_vec_copy_c1(&emitter->pos, &enemy->pos_0x1BC);

    eft_state_flags_set(emitter, 0, 0);
    get_joint_wmat_em(enemy, lbl_8059DCF8[emitter->type], &joint->mtx);

    emitter->retire = fn_80103CB0;
    emitter->advance = fn_80103CEC;

    return emitter;
}

#pragma peephole on

/* Retires the emitter's joint effects and resets its joint count. */
extern "C" void fn_80103CB0(_EFT_EMITTER* emitter)
{
    _EFT_JOINT* joint = emitter->joint;

    push_eft_effect_heap_num(joint->effects, joint->count);
    joint->count = 0;
}

/* Advances the emitter to the next frame's handler. */
extern "C" void fn_80103CEC(_EFT_EMITTER* emitter)
{
    switch (emitter->frame) {
    case 0:
        fn_80103D28(emitter);
        return;
    case 1:
        fn_801041BC(emitter);
        return;
    case 2:
        fn_801048A0(emitter);
        return;
    case 3:
        fn_801048B0(emitter);
        return;
    }
}
