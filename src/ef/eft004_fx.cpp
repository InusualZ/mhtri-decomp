/* ef/eft004_fx.cpp - the eft004 effect family's setters (`eft004_set`, `eft004_set_pl`, `eft004_set_pl2` and two
 *   unnamed siblings), the spawn helper `fn_801007BC`, the record dispatchers and pool releases, and the per-frame
 *   handlers of the family's second half.
 * RANGE. .text 0x80100448-0x80101DF4 (26 functions); extab 0x8000BE74-0x8000BEF4, extabindex 0x80025C98-0x80025D58,
 *   .data 0x8059C528-0x8059C8E8, .sdata 0x80791788-0x80791790, .sdata2 0x807966B8-0x807966E8.  The family's spawner
 *   and first handlers below 0x80100448 are `ef/fn_800FD864_fx.cpp`.
 * FLAGS. `cflags_main`; file-scope `#pragma peephole off` (retail keeps `rlwinm` + `cmpwi` and the hook address in
 *   `r0`; playbook 39), `optimization_level 4` (retail's switch lowering: the `addi`/`cmplwi` range tests of
 *   `fn_80100AA8`/`fn_801011B4`/`fn_80101470`) and `pool_data off`.
 * NAMES. `eft004_set`, `eft004_set_pl` and `eft004_set_pl2` are the runtime dump's own names; `mtx34_trans_get` and
 *   `mtx34_trans_add` are GUESSes from their bodies; the map has only `fn_` stems for the rest.
 *   GUESS (from its body and the note pane's caller): eft004_scaled_spawn
 *   GUESS (from what they hold and who reads them): `eft004_effect_ids`, `eft004_param_ids`, `eft004_rgb_keys_39`,
 *   `eft004_alpha_keys_39`, `eft004_joint_offset_x/y/z`; the callees `get_stg_eft_col_pos`, `se_eft004_req`,
 *   `se_eft004_demo_req`.
 *   GUESS (from each body and its callers): `eft004_key_spawn`.
 * RESIDUALS. None: every row matches and flipcheck is READY.
 * SHAPES. The four-case dispatchers `fn_80100A6C`/`fn_80101774`/`fn_80101DB8` `return` from each case (a `break`
 *   emits a shared trailing branch).
 *   `fn_80100A30`/`fn_80101738` and `fn_80101D70` read `self->work_0x38` through two views (`EftEffectPool`,
 *   `EftHeapPool`); the state handlers read it as `EftPlacePool`/`EftScaledPool`.
 *   `fn_801007BC` takes the rotation block after the three floats (retail sets up `f1`-`f3` before `r7` at every
 *   caller); `eft004_set_pl`/`eft004_set_pl2` pass the owner (`_PLW*`) as its first argument, the others `NULL`.
 *   `fn_80101470`: state 0 ends with `return` (its own `blr`), `case 1: return;` keeps the explicit `cmpwi 1; beqlr`.
 *   `eft004_joint_offset_x/y/z` are `const` objects declared `extern` above their users and defined after them: a
 *   visible initializer folds into the shared float pool, retail keeps them as their own `.sdata2` words.
 *   `fn_801017B0` picks the effect id and kind in an `if`/`else` before one call (retail sets both registers per branch).
 *   The case bodies of each switch follow retail's block order; the locals are declared at the top in the order that
 *   gives retail's registers.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft004_model_set_start.h"
#include "ef/eft007.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/fn_800FD864_fx_types.h"
#include "pl.h" /* MHchar, _PLW */
#include "ef/GameMode_ck.h"
#include "unsplit/unknown.h" /* get_stg_eft_col */
#include "fn_8004CAD8.h" /* cpSetRotMatrix, rotVecY, vec3_scale_in_place (rule 2) */
#include "unsplit/sound.h" /* mhchar_joint_mtx_get */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */
#include "ef/eft001.h" /* mtx34_set_trans */
#include "ef/effect.h"
#include "stage/stg_w.h" /* get_stg_eft_col_pos (rule 2) */
#include "Pl/Pl_master_ck.h"
#include "ef/eft_stage_spawn_flag.h"
#include "ef/eft_res_models_spawn.h"
#include "camera/camera.h" /* get_camera_pos (rule 2) */
#include "Pl/Pl_master_ck.h"
#include "ef/RetireParticleAll.h"
#include "ai/fn_802D44F4.h" /* player_color_get */
#include "unsplit/ef_control.h" /* eft_control (rule 2: the band) */
#include "ef/eft_res_spawn_gate_ck.h"
#include "ef/mtx34_trans_get.h"
#include "ef/mtx34_trans_add.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(Eft004*, s32, s32))eft_state_flags_set)

/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */

/* The pool block `eft004_scaled_spawn` seeds and the player-joint states drive: the count, the pooled effect and
 * its parameter scale. size: 0x0C - lower bound, an approximation */
struct EftScaledPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effect_0x04; /* the pooled effect */
    /* +0x08 */ f32 scale_0x08;
};

/* The pool block `eft004_area_set` fills: a count, twelve heap handles at +0x10, one more at +0x40 and a
 * field at +0x5C. size: 0x60 - lower bound, an approximation */
struct EftHeapPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x10 - 0x04];
    /* +0x10 */ void* entries[12];
    /* +0x40 */ void* handle_0x40;
    /* +0x44 */ u8 unused_0x44[0x5C - 0x44];
    /* +0x5C */ s32 field_0x5C;
};

/* The effect's owner object; only the +0x498 sound handle this unit reads is named.
 * size: 0x49C - lower bound, an approximation */
struct Eft004Owner {
    /* +0x000 */ u8 unused_0x000[0x498];
    /* +0x498 */ s32 handle_0x498;
};

extern "C" void eft_res_slot_release(void* self);
extern "C" void eft_res_models_release(void* block, s32 count);
extern "C" void* eft_res_model_get();
extern "C" Eft004* eft_res_slot_get(u32 pool_id);
extern "C" Eft004* fn_801007BC(_PLW* owner, u8 type, u8 area, u32 param, f32 a, f32 b, f32 c, s32* rot);
extern "C" void fn_80100A30(Eft004* self);
extern "C" void fn_80100A6C(Eft004* self);
extern "C" void fn_80101470(Eft004* self);
extern "C" void fn_80101738(Eft004* self);
extern "C" void fn_80101774(Eft004* self);
extern "C" void fn_80101D70(Eft004* self);
extern "C" void fn_80101DB8(Eft004* self);

/* The 0x24-byte pool block `fn_801007BC` seeds and the state handlers read: the pooled effects, the owner joint they
 * follow (255 = none), their parameter scale, the stage colour and the placement offset (the setters' `b`/`c` are
 * its x and z). size: 0x24 */
struct EftPlacePool {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ u32 joint_0x0C;
    /* +0x10 */ f32 scale_0x10;
    /* +0x14 */ _GXColor color_0x14;
    /* +0x18 */ nw4r::math::VEC3 offset_0x18;
};

/* The joint holder `_PLW::physics_0x13C` points at: the player's `MHchar` sits at +0x04 (a view; the record is
 * `ef/fn_80114E34_types.h`'s `_PLW_PHYSICS`). size: 0x144 - lower bound */
struct EftPlwJoints {
    /* +0x000 */ u8 unused_0x000[0x04];
    /* +0x004 */ MHchar joint_0x004;
};

/* The effect and parameter ids of each eft004 type (0..46). */
u16 eft004_effect_ids[48] = {
    0x000F, 0x00E1, 0x00E2, 0x00E3, 0x0148, 0x0121, 0x0122, 0x0123, 0x0124, 0x0125, 0x0120, 0x0126,
    0x0125, 0x0141, 0x0142, 0x014A, 0x0157, 0x000F, 0x011F, 0x020E, 0x0316, 0x0317, 0x0318, 0x0319,
    0x014A, 0x03AB, 0x0155, 0x0316, 0x0427, 0x0141, 0x00E3, 0x00E7, 0x0152, 0x0121, 0x014A, 0x0142,
    0x0141, 0x05D9, 0x0149, 0x0601, 0x05D9, 0x0316, 0x08F9, 0x0A21, 0x05D9, 0x05D9, 0x014A,
};
u16 eft004_param_ids[48] = {
    0x0000, 0x001A, 0x001A, 0x001A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x001A, 0x001A, 0x001A, 0x0000, 0x0000, 0x0000, 0x001A, 0x0000, 0x0000, 0x0000, 0x0000,
    0x001A, 0x0000, 0x0028, 0x0000, 0x001A, 0x001A, 0x001A, 0x0013, 0x0028, 0x0000, 0x001A, 0x001A,
    0x001A, 0x0000, 0x0000, 0x0025, 0x0000, 0x0000, 0x0000, 0x0013, 0x0000, 0x0000, 0x001A,
};
/* The (frame, r, g, b) and (frame, alpha) keys type 39 recolours its effects by. */
u8 eft004_rgb_keys_39[16] = {0x00, 0xFF, 0xD7, 0x78, 0x02, 0xDE, 0x4C, 0x00, 0x0F, 0x01, 0x01, 0x01, 0xFF, 0x01, 0x01, 0x01};
u8 eft004_alpha_keys_39[8] = {0x00, 0xFF, 0x16, 0xFF, 0x1E, 0x00, 0xFF, 0x00};
/* The offset from the owner's joint 7 the player-joint family places its effect at (defined after its users). */
extern const f32 eft004_joint_offset_x;
extern const f32 eft004_joint_offset_y;
extern const f32 eft004_joint_offset_z;




extern "C" void fn_80100AA8(Eft004* self);
extern "C" void fn_801011B4(Eft004* self);
extern "C" void fn_8010145C(Eft004* self);
extern "C" void fn_8010146C(Eft004* self);
extern "C" void fn_801017B0(Eft004* self);
extern "C" void fn_80101980(Eft004* self);
extern "C" void fn_80101C60(Eft004* self);
extern "C" void fn_80101C70(Eft004* self);
/* eft004_model_set_start / fn_80101FA4 / fn_801025E8 / fn_801025F8 come from their owners' headers (rule 2). */
u8 get_now_areano(); /* target references get_now_areano__Fv: C++ linkage (relocaudit) */
u8 get_now_mapno();

/* `push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count)` - the map's mangled spelling. */
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* `map_se_req(u8, nw4r::math::VEC3*)` - the map's mangled spelling. */
void map_se_req(u8 id, nw4r::math::VEC3* pos);

#pragma peephole off
#pragma optimization_level 4
#pragma pool_data off

/* Public setter: spawns an effect at a vector. */
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag)
{
    s32 params[3];
    Eft004* effect;
    u32 p = param;

    params[0] = 0;
    params[1] = (s32)p;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, flag, 255, scale, 0.0f, 0.0f, params);
    if (effect != NULL) {
        copyVec3(&effect->pos_0x18, pos);
    }
}

/* Public setter: spawns an effect the player owns, at the player's position plus an offset. */
void eft004_set_pl(_PLW* self, u8 type, f32 a, f32 b, f32 c, u32 param)
{
    s32 params[3];
    Eft004* effect;

    params[0] = self->param_0x54;
    params[1] = self->field_0x058 + param;
    params[2] = self->rot_z_0x5C;
    effect = fn_801007BC(self, type, self->area_0x16, 255, a, b, c, params);
    if (effect != NULL) {
        copyVec3(&effect->pos_0x18, &self->vec_0x03C);
        if (type == 17 && (self->field_0x5A4 & 0x6) != 0) {
            eft004_set_pl(self, 2, a, b, c, param);
        }
    }
}

/* Public setter: spawns a player-owned effect with a caller-chosen parameter block. */
void eft004_set_pl2(_PLW* self, u8 type, u32 param, f32 a, f32 b, f32 c, u32 extra)
{
    nw4r::math::VEC3 dir;
    s32 params[3];
    Eft004* effect;

    VEC3_ctor(&dir);
    params[0] = self->param_0x54;
    params[1] = self->field_0x058 + extra;
    params[2] = self->rot_z_0x5C;
    effect = fn_801007BC(self, type, self->area_0x16, param, a, b, c, params);
    if (effect != NULL) {
        copyVec3(&effect->pos_0x18, &self->vec_0x03C);
        if (type == 17 && (self->field_0x5A4 & 0x6) != 0) {
            eft004_set_pl2(self, 2, param, a, b, c, extra);
        }
    }
}

/* Spawns an effect from a vector, then stamps its key byte. */
extern "C" void eft004_key_spawn(u8 type, nw4r::math::VEC3* pos, u32 param, u8 flag, u8 key, f32 scale)
{
    s32 params[3];
    Eft004* effect;

    params[0] = 0;
    params[1] = (s32)param;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, flag, 255, scale, 0.0f, 0.0f, params);
    if (effect != NULL) {
        copyVec3(&effect->pos_0x18, pos);
        effect->byte_0x08 = key;
    }
}

/* Spawns an effect the player owns from a vector, then stamps its key byte. */
extern "C" void fn_8010072C(_PLW* owner, u8 type, nw4r::math::VEC3* pos, u32 param, f32 scale)
{
    s32 params[3];
    Eft004* effect;

    params[0] = 0;
    params[1] = (s32)param;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, owner->effect_key_0x1A4, 255, scale,
                         0.0f, 0.0f, params);
    if (effect != NULL) {
        effect->owner_0x30 = owner;
        copyVec3(&effect->pos_0x18, pos);
        effect->byte_0x08 = 2;
    }
}

/* Builds an eft004 effect record in `area` (the current one; not area 7 of map 21): with an owner, the owner's
 * ground state may refuse the type or turn type 0/17 into 1; then the parameters, rotation and hooks are set. */
extern "C" Eft004* fn_801007BC(_PLW* owner, u8 type, u8 area, u32 param, f32 a, f32 b, f32 c, s32* rot)
{
    _EFT* effect;
    EftPlacePool* work;

    if ((u8)area != (u8)get_now_areano()) {
        return NULL;
    }
    if (get_now_mapno() == 21 && (u8)area == 7) {
        return NULL;
    }
    if (owner != NULL && owner->field_0x001 == 0) {
        return NULL;
    }
    effect = (_EFT*)eft_res_slot_get(36);
    if (effect == NULL) {
        return NULL;
    }
    if (owner != NULL) {
        switch ((u8)type) {
        case 0:
        case 17:
            if ((owner->field_0x5A4 & 6) != 0) {
                if (owner->motion_pos_0x40 < owner->field_0x064 - 50.0f) {
                    fn_8010146C((Eft004*)effect);
                    return NULL;
                }
                type = 1;
            }
            break;
        case 1:
        case 2:
        case 34:
            if ((owner->field_0x5A4 & 6) != 0) {
                if (owner->motion_pos_0x40 < owner->field_0x064 - 50.0f && (param == 20 || param == 17)) {
                    fn_8010146C((Eft004*)effect);
                    return NULL;
                }
            } else {
                fn_8010146C((Eft004*)effect);
                return NULL;
            }
            break;
        case 3:
        case 13:
        case 14:
        case 15:
        case 19:
        case 24:
        case 26:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 35:
        case 36:
        case 43:
        case 46:
            if ((owner->field_0x5A4 & 7) == 0) {
                fn_8010146C((Eft004*)effect);
                return NULL;
            }
            break;
        case 16:
        case 22:
        case 25:
        case 37:
        case 40:
        case 44:
        case 45:
            break;
        default:
            if ((owner->field_0x5A4 & 6) != 0) {
                fn_8010146C((Eft004*)effect);
                return NULL;
            }
            break;
        }
    }
    work = (EftPlacePool*)effect->work_0x38;
    work->count = 1;
    effect->field_0x03 = 4;
    effect->type_0x02 = type;
    work->joint_0x0C = param;
    work->scale_0x10 = a;
    work->offset_0x18.x = b;
    work->offset_0x18.z = c;
    effect->source_0x30 = owner;
    effect->rot_0x24.x = rot[0];
    effect->rot_0x24.y = rot[1];
    effect->rot_0x24.z = rot[2];
    effect->area_0x44 = area;
    effect->field_0x10 = 0;
    eft_state_flags_set_c1((Eft004*)effect, 0, 0);
    effect->release_0x40 = (void (*)(_EFT*))fn_80100A30;
    effect->dispatch_0x34 = (void (*)(_EFT*))fn_80100A6C;
    return (Eft004*)effect;
}

/* Hands the effect pool block back to the effect heap and clears its count. */
extern "C" void fn_80100A30(Eft004* self)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;

    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80100A6C(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80100AA8(self);
        return;
    case 1:
        fn_801011B4(self);
        return;
    case 2:
        fn_8010145C(self);
        return;
    case 3:
        fn_8010146C(self);
        return;
    }
}

/* State 0: creates the type's pooled effect(s) (the in-demo ids for some types), places them on the owner's joint
 * or at the record's position plus the rotated offset, picks the stage colour and the height, then runs state 1. */
extern "C" void fn_80100AA8(Eft004* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 scale;
    u16 param_id;
    u16 effect_id;
    EftPlacePool* work;
    _PLW* owner;
    s32 i;
    u32 col;
    _EFT* eft = (_EFT*)self;

    MTX34_ctor(&mtx);
    VEC3_ctor(&offset);
    VEC3_ctor(&scale);
    work = (EftPlacePool*)self->work_0x38;
    if (self->byte_0x08 != 2) {
        owner = (_PLW*)self->owner_0x30;
    } else {
        owner = NULL;
    }
    self->state_0x05++;
    effect_id = eft004_effect_ids[self->type_0x02];
    param_id = eft004_param_ids[self->type_0x02];
    switch (self->type_0x02) {
    case 0:
        if (GameMode_ck() == 2) {
            effect_id = 273;
            param_id = 0;
        } else {
            param_id = 0;
        }
        break;
    case 1:
        if (GameMode_ck() == 2) {
            effect_id = 998;
            param_id = 20;
        }
        break;
    case 2:
        if (GameMode_ck() == 2) {
            effect_id = 999;
            param_id = 20;
        }
        break;
    case 5:
        if (GameMode_ck() == 2) {
            effect_id = 1695;
            param_id = 0;
        }
        break;
    case 10:
        if (GameMode_ck() == 2) {
            effect_id = 1002;
            param_id = 0;
        }
        break;
    case 15:
        if (GameMode_ck() == 2) {
            effect_id = 1001;
            param_id = 20;
        }
        break;
    case 17:
        if (GameMode_ck() == 2) {
            effect_id = 273;
            param_id = 0;
        }
        break;
    }
    for (i = 0; i < work->count; i++) {
        work->effects[i] = res_eft_create(effect_id, param_id, 0);
        if (work->effects[i] == NULL) {
            fn_8010146C(self);
            return;
        }
    }
    switch (self->type_0x02) {
    case 34:
        work->effects[1] = res_eft_create(226, param_id, 0);
        if (work->effects[1] == NULL) {
            fn_8010146C(self);
            return;
        }
        work->count++;
        break;
    case 35:
    case 36:
        work->effects[1] = res_eft_create(227, param_id, 0);
        if (work->effects[1] == NULL) {
            fn_8010146C(self);
            return;
        }
        work->count++;
        break;
    case 44:
    case 45:
        work->effects[1] = res_eft_create(1497, param_id, 0);
        if (work->effects[1] == NULL) {
            fn_8010146C(self);
            return;
        }
        work->count++;
        break;
    case 46:
        work->effects[1] = res_eft_create(227, param_id, 0);
        if (work->effects[1] == NULL) {
            fn_8010146C(self);
            return;
        }
        work->count++;
        break;
    }
    if (work->joint_0x0C != 255) {
        ((EftPlwJoints*)owner->physics_0x13C)->joint_0x004.get_joint_wpos(work->joint_0x0C, &self->pos_0x18);
    }
    switch (self->type_0x02) {
    case 23:
        setVector3(&scale, 1.0f, 0.5f, 1.0f);
        vec3_scale_in_place(&scale, work->scale_0x10);
        change_paramscale_eff_vec3(work->effects[0], &scale);
        break;
    case 33:
        setVector3(&scale, 3.2f, 2.5f, 3.2f);
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff_vec3(work->effects[i], &scale);
        }
        break;
    case 44:
    case 45:
        change_paramscale_eff(work->effects[0], work->scale_0x10);
        change_paramscale_eff(work->effects[1], 0.5f * work->scale_0x10);
        break;
    default:
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff(work->effects[i], work->scale_0x10);
        }
        break;
    }
    fn_80101470(self);
    if ((s32)self->type_0x02 == 20 || (s32)self->type_0x02 == 24) {
        if (owner != NULL) {
            mhchar_joint_mtx_get(&((EftPlwJoints*)owner->physics_0x13C)->joint_0x004, work->joint_0x0C,
                                 (Mtx34*)&mtx);
            copyVec3(&offset, &work->offset_0x18);
            mulVecMat(&offset, &mtx);
            addVec3To(&self->pos_0x18, &offset);
            switch (self->type_0x02) {
            case 20:
                self->pos_0x18.y = 5.0f + owner->ground_y_0x060;
                break;
            case 24:
                self->pos_0x18.y = 5.0f + owner->field_0x064;
                break;
            }
        }
    } else {
        cpSetRotMatrix(&eft->rot_0x24, &mtx);
        copyVec3(&offset, &work->offset_0x18);
        rotVecY(&offset, eft->rot_0x24.y);
        addVec3To(&self->pos_0x18, &offset);
    }
    switch (self->type_0x02) {
    case 1:
    case 2:
    case 3:
    case 13:
    case 14:
    case 15:
    case 19:
    case 24:
    case 26:
    case 29:
    case 30:
    case 32:
    case 34:
    case 35:
    case 36:
    case 40:
    case 45:
    case 46:
        col = get_stg_eft_col(self->area_0x44, 1);
        work->color_0x14.r = (col & 0xFF000000) >> 24;
        work->color_0x14.g = (col & 0xFF0000) >> 16;
        work->color_0x14.b = (col & 0xFF00) >> 8;
        work->color_0x14.a = col;
        if (owner != NULL) {
            self->pos_0x18.y = 5.0f + owner->field_0x064;
        }
        break;
    case 0:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 16:
    case 17:
    case 18:
    case 20:
    case 21:
    case 22:
    case 23:
    case 25:
    case 27:
    case 33:
    case 37:
    case 38:
    case 39:
    case 41:
    case 42:
    case 44:
        if ((s32)get_now_mapno() == 9) {
            col = get_stg_eft_col_pos(self->area_0x44, &self->pos_0x18);
        } else {
            col = get_stg_eft_col(self->area_0x44, 0);
        }
        work->color_0x14.r = (col & 0xFF000000) >> 24;
        work->color_0x14.g = (col & 0xFF0000) >> 16;
        work->color_0x14.b = (col & 0xFF00) >> 8;
        work->color_0x14.a = col;
        if (owner != NULL) {
            self->pos_0x18.y = owner->ground_y_0x060;
        }
        break;
    case 28:
        col = get_stg_eft_col(self->area_0x44, 1);
        work->color_0x14.r = (col & 0xFF000000) >> 24;
        work->color_0x14.g = (col & 0xFF0000) >> 16;
        work->color_0x14.b = (col & 0xFF00) >> 8;
        work->color_0x14.a = col;
        if (owner != NULL) {
            self->pos_0x18.y = owner->field_0x064 - 10.0f;
        }
        break;
    case 31:
        col = get_stg_eft_col(self->area_0x44, 1);
        work->color_0x14.r = (col & 0xFF000000) >> 24;
        work->color_0x14.g = (col & 0xFF0000) >> 16;
        work->color_0x14.b = (col & 0xFF00) >> 8;
        work->color_0x14.a = col;
        break;
    case 43:
        col = get_stg_eft_col(self->area_0x44, 1);
        work->color_0x14.r = (col & 0xFF000000) >> 24;
        work->color_0x14.g = (col & 0xFF0000) >> 16;
        work->color_0x14.b = (col & 0xFF00) >> 8;
        work->color_0x14.a = col;
        break;
    default:
        work->color_0x14.r = 255;
        work->color_0x14.g = 255;
        work->color_0x14.b = 255;
        work->color_0x14.a = 255;
        break;
    }
    mtx34_set_trans(&mtx, &self->pos_0x18);
    if (self->type_0x02 != 31) {
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
    }
    self->flag_0x01 = 1;
    fn_801011B4(self);
}

/* State 1: follows the owner's joint (type 31), moves the pooled effects and recolours them (type 39 from its key
 * tables), ends when every effect has stopped, and otherwise hands them to the model spawner (types 28-30 only on
 * their side of the camera's y 0). */
extern "C" void fn_801011B4(Eft004* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 offset;
    s32 i;
    u32 mode;
    u8 stopped;
    _PLW* owner;
    EftPlacePool* work;

    stopped = 0;
    work = (EftPlacePool*)self->work_0x38;
    owner = (_PLW*)self->owner_0x30;
    MTX34_ctor(&mtx);
    VEC3_ctor(&offset);
    if ((s32)self->type_0x02 == 31) {
        mhchar_joint_mtx_get(&((EftPlwJoints*)owner->physics_0x13C)->joint_0x004, work->joint_0x0C, (Mtx34*)&mtx);
        copyVec3(&offset, &work->offset_0x18);
        mulVecMat(&offset, &mtx);
        mtx34_trans_add(&mtx, &offset);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        SetRootMtxTrans(work->effects[0], &self->pos_0x18);
    }
    for (i = 0; i < work->count; i++) {
        if (effect_move(work->effects[i]) == 0) {
            stopped++;
        }
        switch (self->type_0x02) {
        case 44:
        case 45:
        case 16:
        case 37:
        case 40:
            break;
        case 39:
            self->field_0x10++;
            eftGetKeyRGB(eft004_rgb_keys_39, self->field_0x10, &work->color_0x14.r, &work->color_0x14.g,
                         &work->color_0x14.b);
            work->color_0x14.a = eftGetKeyAlpha(eft004_alpha_keys_39, self->field_0x10);
            change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x14);
            break;
        default:
            change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x14);
            break;
        }
    }
    if (stopped == work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    mode = 1;
    if (owner != NULL && Pl_master_ck(owner) == 1) {
        mode = eft_stage_spawn_flag(owner) | 1;
    }
    switch (self->type_0x02) {
    case 28:
        if (get_camera_pos().y < 0.0f) {
            eft_res_models_spawn((_EFT*)self, (void**)work->effects, mode, work->count, NULL);
        }
        break;
    case 29:
    case 30:
        if (get_camera_pos().y > 0.0f) {
            eft_res_models_spawn((_EFT*)self, (void**)work->effects, mode, work->count, NULL);
        }
        break;
    default:
        eft_res_models_spawn((_EFT*)self, (void**)work->effects, mode, work->count, NULL);
        break;
    }
}

/* Copies a matrix's translation column into a vector. */
extern "C" void mtx34_trans_get(nw4r::math::MTX34* mtx, nw4r::math::VEC3* out)
{
    out->x = mtx->m[0][3];
    out->y = mtx->m[1][3];
    out->z = mtx->m[2][3];
}

/* Adds a vector to a matrix's translation column. */
extern "C" void mtx34_trans_add(nw4r::math::MTX34* mtx, nw4r::math::VEC3* v)
{
    mtx->m[0][3] += v->x;
    mtx->m[1][3] += v->y;
    mtx->m[2][3] += v->z;
}

/* Bumps the effect's state index. */
extern "C" void fn_8010145C(Eft004* self)
{
    self->state_0x05++;
}

/* Destroys an effect record. */
extern "C" void fn_8010146C(Eft004* self)
{
    eft_res_slot_release(self);
}

/* Plays the footstep/voice sound the effect's state and type select. */
extern "C" void fn_80101470(Eft004* self)
{
    s32 state = self->byte_0x08;

    switch (state) {
    case 0:
        switch (self->type_0x02) {
        case 1: case 14: case 15: case 24: case 26: case 34:
            se_slot_req_pos(0, 19, &self->pos_0x18);
            break;
        case 13: case 29: case 32:
            se_slot_req_pos(0, 20, &self->pos_0x18);
            break;
        case 35: case 46:
            se_slot_req_pos(0, 37, &self->pos_0x18);
            break;
        case 39:
            map_se_req(11, &self->pos_0x18);
            break;
        }
        break;
    case 1:
        return;
    case 2:
    case 3:
        switch (self->type_0x02) {
        case 1: case 14: case 15: case 24: case 26: case 34:
            se_slot_req_pos(0, 19, &self->pos_0x18);
            break;
        case 13: case 29: case 35:
            switch (state) {
            case 2:
                se_slot_req_pos(0, 37, &self->pos_0x18);
                break;
            case 3:
                se_slot_req_pos(0, 19, &self->pos_0x18);
                break;
            }
            break;
        case 42:
            fn_800DCF0C(((Eft004Owner*)self->owner_0x30)->handle_0x498, &self->pos_0x18);
            break;
        case 46:
            se_slot_req_pos(0, 37, &self->pos_0x18);
            break;
        case 39:
            map_se_req(11, &self->pos_0x18);
            break;
        }
        break;
    }
}

/* Creates a player-owned effect with a 1-effect pool, seeding its area and key byte. */
extern "C" void eft004_hold_item_spawn(_PLW* self)
{
    Eft004* effect = eft_res_slot_get(12);

    if (effect != NULL) {
        EftEffectPool* pool = (EftEffectPool*)effect->work_0x38;
        u8 key;

        pool->count = 1;
        effect->phase_0x03 = 5;
        effect->type_0x02 = 0;
        effect->timer_0x0C = 0;
        effect->field_0x10 = 20;
        effect->owner_0x30 = self;
        effect->area_0x44 = self->area_0x16;
        if (GameMode_ck() == 1) {
            key = self->field_0x655;
            if ((key & 0x80) != 0) {
                effect->byte_0x08 = 0;
            } else {
                effect->byte_0x08 = key;
            }
        } else {
            effect->byte_0x08 = 0;
        }
        eft_state_flags_set_c1(effect, 1, 0);
        effect->release_0x40 = fn_80101738;
        effect->dispatch_0x34 = fn_80101774;
    }
}

/* Creates a player-owned effect with a 1-effect pool, at a caller-chosen position and scale. */
extern "C" void eft004_scaled_spawn(nw4r::math::VEC3* pos, u8 area, f32 scale)
{
    Eft004* effect = eft_res_slot_get(12);

    if (effect != NULL) {
        EftScaledPool* pool = (EftScaledPool*)effect->work_0x38;

        pool->count = 1;
        pool->scale_0x08 = scale;
        effect->phase_0x03 = 5;
        effect->type_0x02 = 1;
        effect->owner_0x30 = NULL;
        effect->area_0x44 = area;
        copyVec3(&effect->pos_0x18, pos);
        effect->timer_0x0C = 0;
        effect->field_0x10 = 0;
        eft_state_flags_set_c1(effect, 1, 0);
        effect->release_0x40 = fn_80101738;
        effect->dispatch_0x34 = fn_80101774;
    }
}

/* Hands the effect pool block back to the effect heap and clears its count. */
extern "C" void fn_80101738(Eft004* self)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;

    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80101774(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_801017B0(self);
        return;
    case 1:
        fn_80101980(self);
        return;
    case 2:
        fn_80101C60(self);
        return;
    case 3:
        fn_80101C70(self);
        return;
    }
}

/* State 0 of the player-joint family: creates the effect (the demo id in demo mode), places type 0 on the owner's
 * joint 7 with its offset and type 1 at the record's position, plays the spawn sound, then runs state 1. */
extern "C" void fn_801017B0(Eft004* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    s32 i;
    _PLW* owner;
    EftEffectPool* work;
    u16 id;
    u16 kind;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    work = (EftEffectPool*)self->work_0x38;
    owner = (_PLW*)self->owner_0x30;
    self->state_0x05++;
    if (GameMode_ck() == 2) {
        id = 2598;
        kind = 1;
    } else {
        id = 16;
        kind = 1;
    }
    work->effects[0] = res_eft_create(id, kind, 0);
    if (work->effects[0] == NULL) {
        fn_80101C70(self);
        return;
    }
    switch (self->type_0x02) {
    case 0:
        self->flag_0x01 = owner->field_0x001;
        setVector3(&pos, eft004_joint_offset_x, eft004_joint_offset_y, eft004_joint_offset_z);
        mhchar_joint_mtx_get(&((EftPlwJoints*)owner->physics_0x13C)->joint_0x004, 7, (Mtx34*)&mtx);
        mulVecMat(&pos, &mtx);
        self->pos_0x18.x = mtx.m[0][3] += pos.x;
        self->pos_0x18.y = mtx.m[1][3] += pos.y;
        self->pos_0x18.z = mtx.m[2][3] += pos.z;
        if (self->area_0x44 == (u8)get_now_areano() && self->flag_0x01 != 0) {
            se_eft004_req(&self->pos_0x18);
        }
        break;
    case 1:
        self->flag_0x01 = 1;
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        }
        if (GameMode_ck() == 2 && self->area_0x44 == (u8)get_now_areano()) {
            se_eft004_demo_req(&self->pos_0x18);
        }
        break;
    }
    fn_80101980(self);
}

/* State 1 of the player-joint family: type 0 follows the owner's joint 7 while the owner is in one of its acts (when
 * the effect has stopped it is retired and re-created), recoloured in the player's colour in multiplayer; type 1 runs
 * until its effect stops.  The live effect goes to the model spawner. */
extern "C" void fn_80101980(Eft004* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    EftScaledPool* work;
    s32 i;
    _PLW* owner;
    nw4r::ef::EffectSystem* system;
    u32 col;
    _GXColor color;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    owner = (_PLW*)self->owner_0x30;
    work = (EftScaledPool*)self->work_0x38;
    system = eft_control.effect_system_0x04;
    switch (self->type_0x02) {
    case 0:
        if (eft_res_spawn_gate_ck((_EFT*)self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        self->flag_0x01 = owner->field_0x001;
        self->area_0x44 = owner->area_0x16;
        if (--self->field_0x10 > 0) {
            return;
        }
        if (GameMode_ck() == 1) {
            if (Pl_act_ck(owner, 0, 20) == 0 && Pl_act_ck(owner, 0, 158) == 0) {
                self->flag_0x01 = 0;
                self->state_0x05++;
                return;
            }
        } else if (Pl_act_ck(owner, 0, 1) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        setVector3(&pos, eft004_joint_offset_x, eft004_joint_offset_y, eft004_joint_offset_z);
        mhchar_joint_mtx_get(&((EftPlwJoints*)owner->physics_0x13C)->joint_0x004, 7, (Mtx34*)&mtx);
        mulVecMat(&pos, &mtx);
        self->pos_0x18.x = mtx.m[0][3] += pos.x;
        self->pos_0x18.y = mtx.m[1][3] += pos.y;
        self->pos_0x18.z = mtx.m[2][3] += pos.z;
        for (i = 0; i < work->count; i++) {
            (&work->effect_0x04)[i]->SetRootMtx(mtx);
        }
        if (effect_move(work->effect_0x04) == 0) {
            RetireParticleAll(work->effect_0x04);
            work->effect_0x04->RetireEmitterAll();
            if (system != NULL) {
                system->RetireEffect(work->effect_0x04);
            }
            self->state_0x05 = 0;
            self->field_0x10 = 10;
            fn_801017B0(self);
            return;
        }
        if (GameMode_ck() == 1) {
            col = player_color_get(self->byte_0x08);
            color.r = col >> 24;
            color.g = (col >> 16) & 0xFF;
            color.b = (col >> 8) & 0xFF;
            color.a = col;
            change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        }
        break;
    case 1:
        change_paramscale_eff(work->effect_0x04, work->scale_0x08);
        if (effect_move(work->effect_0x04) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        break;
    }
    eft_res_models_spawn((_EFT*)self, (void**)&work->effect_0x04, 1, work->count, NULL);
}

const f32 eft004_joint_offset_x = 10.0f;
const f32 eft004_joint_offset_y = -8.0f;
const f32 eft004_joint_offset_z = 5.0f;

/* Bumps the effect's state index. */
extern "C" void fn_80101C60(Eft004* self)
{
    self->state_0x05++;
}

/* Destroys an effect record. */
extern "C" void fn_80101C70(Eft004* self)
{
    eft_res_slot_release(self);
}

/* Creates an effect with a twelve-entry heap pool and one extra handle; destroys it on any failure. */
extern "C" void eft004_area_set(u32 type)
{
    s32 i;
    Eft004* effect;
    EftHeapPool* pool;
    u32 area;

    area = get_now_areano();
    effect = eft_res_slot_get(0x60);
    if (effect != NULL) {
        effect->type_0x02 = type;
        effect->release_0x40 = fn_80101D70;
        effect->dispatch_0x34 = fn_80101DB8;
        pool = (EftHeapPool*)effect->work_0x38;
        pool->count = 12;
        for (i = 0; i < pool->count; i++) {
            pool->entries[i] = eft_res_model_get();
            if (pool->entries[i] == NULL) {
                eft_res_slot_release(effect);
                return;
            }
        }
        pool->field_0x5C = 0;
        pool->handle_0x40 = eft_res_model_get();
        if (pool->handle_0x40 == NULL) {
            eft_res_slot_release(effect);
            return;
        }
        effect->phase_0x03 = 6;
        effect->area_0x44 = area;
        eft_state_flags_set_c1(effect, 0, 0);
    }
}

/* Releases the pool's twelve heap handles and its single extra handle, then clears its count. */
extern "C" void fn_80101D70(Eft004* self)
{
    EftHeapPool* pool = (EftHeapPool*)self->work_0x38;

    eft_res_models_release(pool->entries, pool->count);
    eft_res_models_release(&pool->handle_0x40, 1);
    pool->count = 0;
}

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80101DB8(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        eft004_model_set_start(self);
        return;
    case 1:
        fn_80101FA4(self);
        return;
    case 2:
        fn_801025E8(self);
        return;
    case 3:
        fn_801025F8(self);
        return;
    }
}
