/* ef/eft004_fx.cpp - effect 004 fx band
 *
 * `.text` 0x80100448..0x80101DF4, 21 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit, built from `ef/eft004.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source.
 */

#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/fn_80101DF4.h"
#include "ef/eft007.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/fn_800FD864_fx_types.h"
#include "Pl/plw.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(Eft004*, s32, s32))eft_state_flags_set)

/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */

/* The pool block `fn_80101670` seeds: the count and a scale. size: 0x0C - lower bound, an approximation */
struct EftScaledPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ f32 scale_0x08;
};

/* The pool block `fn_80101C74` fills: a count, twelve heap handles at +0x10, one more at +0x40 and a
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
extern "C" void fn_800F8A44(void* block, s32 count);
extern "C" void* eft_res_model_get();
extern "C" Eft004* eft_res_slot_get(u32 pool_id);
extern "C" Eft004* fn_801007BC(void* owner, u32 arg1, u32 arg2, u32 arg3, s32* arg4,
                               f32 farg0, f32 farg1, f32 farg2);

extern "C" u8 GameMode_ck(Eft004* self);
extern "C" f32 lbl_807966B8; /* 0.0f  .sdata2 */

extern "C" void fn_80100AA8(Eft004* self);
extern "C" void fn_801011B4(Eft004* self);
extern "C" void fn_8010145C(Eft004* self);
extern "C" void fn_8010146C(Eft004* self);
extern "C" void fn_801017B0(Eft004* self);
extern "C" void fn_80101980(Eft004* self);
extern "C" void fn_80101C60(Eft004* self);
extern "C" void fn_80101C70(Eft004* self);
/* fn_80101DF4 / fn_80101FA4 / fn_801025E8 / fn_801025F8 come from their owners' headers (rule 2). */
u8 get_now_areano(); /* target references get_now_areano__Fv: C++ linkage (relocaudit) */

/* `push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count)` - the map's mangled spelling. */
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

/* `map_se_req(u8, nw4r::math::VEC3*)` - the map's mangled spelling. */
void map_se_req(u8 id, nw4r::math::VEC3* pos);

#pragma peephole off

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

/* Hands the effect pool block back to the effect heap and clears its count. */
extern "C" void fn_80100A30(Eft004* self)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;

    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* Hands the effect pool block back to the effect heap and clears its count. */
extern "C" void fn_80101738(Eft004* self)
{
    EftEffectPool* pool = (EftEffectPool*)self->work_0x38;

    push_eft_effect_heap_num(pool->effects, pool->count);
    pool->count = 0;
}

/* Releases the pool's twelve heap handles and its single extra handle, then clears its count. */
extern "C" void fn_80101D70(Eft004* self)
{
    EftHeapPool* pool = (EftHeapPool*)self->work_0x38;

    fn_800F8A44(pool->entries, pool->count);
    fn_800F8A44(&pool->handle_0x40, 1);
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

/* Per-frame dispatcher: runs the body for the effect's current state. */
extern "C" void fn_80101DB8(Eft004* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80101DF4(self);
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

/* Spawns an effect from a vector, then stamps its key byte. */
extern "C" void fn_801006A0(u8 type, nw4r::math::VEC3* pos, u32 param, u8 flag, u8 key, f32 scale)
{
    s32 params[3];
    Eft004* effect;

    params[0] = 0;
    params[1] = (s32)param;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, flag, 255, params, scale, lbl_807966B8, lbl_807966B8);
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
    effect = fn_801007BC(NULL, type, owner->effect_key_0x1A4, 255, params, scale,
                         lbl_807966B8, lbl_807966B8);
    if (effect != NULL) {
        effect->owner_0x30 = owner;
        copyVec3(&effect->pos_0x18, pos);
        effect->byte_0x08 = 2;
    }
}

/* Public setter: spawns an effect at a vector. */
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag)
{
    s32 params[3];
    Eft004* effect;
    u32 p = param;

    params[0] = 0;
    params[1] = (s32)p;
    params[2] = 0;
    effect = fn_801007BC(NULL, type, flag, 255, params, scale, lbl_807966B8, lbl_807966B8);
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
    effect = fn_801007BC(self, type, self->area_0x16, 255, params, a, b, c);
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
    effect = fn_801007BC(self, type, self->area_0x16, param, params, a, b, c);
    if (effect != NULL) {
        copyVec3(&effect->pos_0x18, &self->vec_0x03C);
        if (type == 17 && (self->field_0x5A4 & 0x6) != 0) {
            eft004_set_pl2(self, 2, param, a, b, c, extra);
        }
    }
}

/* Creates a player-owned effect with a 1-effect pool, seeding its area and key byte. */
extern "C" void fn_80101594(_PLW* self)
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
        if (GameMode_ck(effect) == 1) {
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
extern "C" void fn_80101670(nw4r::math::VEC3* pos, u8 area, f32 scale)
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

/* Creates an effect with a twelve-entry heap pool and one extra handle; destroys it on any failure. */
extern "C" void fn_80101C74(u8 type)
{
    u8 area = get_now_areano();
    Eft004* effect = eft_res_slot_get(0x60);

    if (effect != NULL) {
        EftHeapPool* pool;
        s32 i;

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

/* Plays the footstep/voice sound the effect's state and type select. */
extern "C" void fn_80101470(Eft004* self)
{
    s32 state = self->byte_0x08;

    switch (state) {
    case 0:
        switch (self->type_0x02) {
        case 1: case 14: case 15: case 24: case 26: case 34:
            fn_800DA72C(0, 19, &self->pos_0x18);
            break;
        case 13: case 29: case 32:
            fn_800DA72C(0, 20, &self->pos_0x18);
            break;
        case 35: case 46:
            fn_800DA72C(0, 37, &self->pos_0x18);
            break;
        case 39:
            map_se_req(11, &self->pos_0x18);
            break;
        }
        break;
    case 2:
    case 3:
        switch (self->type_0x02) {
        case 1: case 14: case 15: case 24: case 26: case 34:
            fn_800DA72C(0, 19, &self->pos_0x18);
            break;
        case 13: case 29: case 35:
            if (state == 2) {
                fn_800DA72C(0, 37, &self->pos_0x18);
            } else {
                fn_800DA72C(0, 19, &self->pos_0x18);
            }
            break;
        case 39:
            map_se_req(11, &self->pos_0x18);
            break;
        case 42:
            fn_800DCF0C(((Eft004Owner*)self->owner_0x30)->handle_0x498, &self->pos_0x18);
            break;
        case 46:
            fn_800DA72C(0, 37, &self->pos_0x18);
            break;
        }
        break;
    }
}
