/* ef/eft001.cpp - the `eft001` effect cluster, 0x800FAE08..0x800FCED4 (23 functions).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every
 * fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt; the one non-fn_ name,
 * eft001_set_pos__FPQ34nw4r4math4VEC3UlP10_CP_VECTORUcf, is the unit's own runtime-dump name).
 *
 * Registered from proposal/800FAE08_fn_800FAE08.cpp.  The name is `eft001`: the range's only real
 * runtime-dump symbol is `eft001_set_pos` (dumpmap lookup 0x800FC250), and its family is the
 * neighbouring ef/eft002.cpp / eft004.cpp / eft007.cpp / eft009.cpp units.  C++ from that mangled
 * definition (langcheck: conclusive) and from the mangled undefined callees.
 *
 * The unit is the model-placement half of the eft001 effect: a 0x48-byte `_EFT` record (ef.h) whose
 * +0x38 pool block carries the pooled effect and its `MHchar` model(s), and a two-state machine
 * (create -> per-frame place -> retire) that recolours and re-poses the model on an enemy joint or
 * the player.
 *
 * Every `fn_XXXXXXXX` callee is `extern "C"` so the compiler emits the map's spelling; the mangled
 * callees (`ran_suu__Fl`, `setVector3__FP...`, the `MHchar` members, ...) come from real C++
 * declarations (docs/plan.md 6.5 rule 9).
 *
 * Residual: the bodies below are reconstructed one at a time; the functions still stubbed and what
 * was measured are in the unit report and the `.pi/notes/800fae08-fn-800fae08-d3c2.md` note.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/fn_800D7F54.h"
#include "ef/eft002.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the +0x38 pool block, in its two per-family views (ef.h's `_EFT_WORK` is the generic 0x10 prefix)
 * ------------------------------------------------------------------------------------------------- */

/* The `fn_800FBD68` / `fn_800FC27C` family block: the per-effect model list and the K-colour the
 * frame writes before it hands off. size: 0x34 - lower bound, an approximation (the block
 * continues past the offsets this unit reads). */
struct _EFT001_MODEL_WORK {
    /* +0x00 */ s32 count;          /* the number of MHchar entries at +0x04 */
    /* +0x04 */ MHchar* models[2];  /* the joint models the state machine places */
    /* +0x0C */ f32 scale_0x0C;     /* the parameter scale the effect was built with */
    /* +0x10 */ u8 unused_0x10[0x08];
    /* +0x18 */ s8 timer_0x18;      /* the per-frame countdown the frame handler runs down */
    /* +0x19 */ u8 color_0x19[4];   /* the K-colour passed to setTevKColor */
    /* +0x1D */ u8 unused_0x1D[0x17];
};

/* The `fn_800FBE64` / `fn_800FC484` family block: two pooled effects and the model they share. */
struct _EFT001_EFFECT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effect_0x04;  /* the primary pooled effect */
    /* +0x08 */ nw4r::ef::Effect* effect_0x08;  /* the secondary effect / the joint model (cast per family) */
    /* +0x0C */ f32 scale_0x0C;
    /* +0x10 */ u8 unused_0x10[4];
    /* +0x14 */ u8 color_0x14[4];
    /* +0x18 */ union { f32 value_0x18; s8 timer_0x18; };
    /* +0x1C */ u8 unused_0x1C[0x0C];
    /* +0x28 */ u8 flag_0x28;
    /* +0x29 */ u8 flag_0x29;
    /* +0x2A */ u8 unused_0x2A[2];
    /* +0x2C */ s32 value_0x2C;
    /* +0x30 */ s32 value_0x30;
};
/* size: 0x34 - lower bound, an approximation. */

/* The opaque source record the `fn_800FBE64` / `fn_800FC1DC` builders read: the area byte at +0x0F
 * and the joint/parameter word at +0x36.  Only the offsets this unit reads are named.
 * size: 0x3C - lower bound, an approximation. */
struct _EFT001_SRC {
    /* +0x00 */ u8 unused_0x00[0x0F];
    /* +0x0F */ u8 area_0x0F;      /* the area the builder is gated on */
    /* +0x10 */ u8 unused_0x10[0x26];
    /* +0x36 */ u16 param_0x36;     /* the joint/parameter passed to the model builder */
    /* +0x38 */ u8 unused_0x38[0x04];
};

/* The joint holder `_PLW::physics_0x13C` points at: the `MHchar` the effect is placed on sits at
 * +0x04 (eft007 reads the same record as `joints_0x13C->joint_0x004`).  size: 0x144 - lower bound. */
struct _EFT001_PHYSICS {
    /* +0x000 */ u8 unused_0x000[4];
    /* +0x004 */ MHchar joint_0x004;
};

/* The `_PLW` fields `fn_800FBBC0` reads, at their offsets (a view: `pl.h`'s canonical record still
 * spells +0x0C `unk00C`, which rule 7 forbids in this file).  size: 0x140 - lower bound. */
struct _EFT001_PLW_VIEW {
    /* +0x000 */ u8 unused_0x000[0x0A];
    /* +0x00A */ u8 field_0x00A;      /* selects the pose branch (2 = airborne variant, 12 = grounded) */
    /* +0x00B */ u8 unused_0x00B;
    /* +0x00C */ u16 field_0x00C;     /* the ground/step state the pose branch gates on */
    /* +0x00E */ u8 unused_0x00E[0x52];
    /* +0x060 */ f32 field_0x060;     /* the player's base y the effect sits on */
    /* +0x064 */ u8 unused_0x064[0xD8];
    /* +0x13C */ void* physics_0x13C; /* the joint holder the effect is placed on */
};

/* One particle-manager entry `fn_800FCEC8` walks: the manager sits after an 88-byte header.
 * size: 0x59 - lower bound. */
struct _EFT001_PM_ENTRY {
    /* +0x00 */ u8 unused_0x00[0x58];
    /* +0x58 */ u8 field_0x58;
};

/* The enemy work record `fn_800FAE08` drives: only the offsets the state machine reads are named
 * (`_ENEMY_WORK`'s canonical view names fewer of them).  A unit-local view of the same record, cast
 * from `_EFT::source_0x30` where the effect is attached to an enemy.  size: 0x22C - lower bound. */
struct _EFT001_ENEMY {
    /* +0x000 */ u8 unused_0x000[0x188];
    /* +0x188 */ nw4r::math::VEC3 pos_0x188;
    /* +0x194 */ u8 unused_0x194[0x1E2 - 0x194];
    /* +0x1E2 */ u8 state_0x1E2;       /* the action sub-state the pose gate reads */
    /* +0x1E3 */ u8 unused_0x1E3[0x204 - 0x1E3];
    /* +0x204 */ u32 joint_0x204;      /* the joint id `get_joint_wpos_em` places on (0xFFFFFFFF = the root) */
    /* +0x208 */ u8 unused_0x208[0x20C - 0x208];
    /* +0x20C */ f32 value_0x20C;      /* the ground-drop y for a grounded pose */
    /* +0x210 */ f32 value_0x210;      /* the base y for an airborne pose */
    /* +0x214 */ u8 unused_0x214[0x228 - 0x214];
    /* +0x228 */ u16 flags_0x228;
};

/* ---------------------------------------------------------------------------------------------------
 * the mangled callees (real C++ declarations - rule 9: the member call, never the mangled spelling)
 * ------------------------------------------------------------------------------------------------- */

u32 get_now_areano();
s32 ran_suu(s32 range);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
nw4r::ef::Effect* res_eft_create(u16 id, u16 param, u32 idx);
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
void change_paramscale_eff_vec3(nw4r::ef::Effect* effect, nw4r::math::VEC3* vec);
u32 effect_move(nw4r::ef::Effect* effect);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);
u8 eftGetKeyAlpha(u8* key, long frame);
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
void mulVecMat(nw4r::math::VEC3* v, nw4r::math::MTX34* m);
u32 calcVecAng3(nw4r::math::VEC3* v);
void rotLocalMatX(u32 angle, nw4r::math::MTX34* m);
void rotLocalMatY(u32 angle, nw4r::math::MTX34* m);
void rotVecY(nw4r::math::VEC3* v, u32 angle);
void get_joint_wpos_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::VEC3* out);
extern "C" f32 fn_80135998(void* enemy); /* target references the plain name (relocaudit) */
f32 GetGroundHit(nw4r::math::VEC3* pos, u32 ground, u8 flag);

/* The unmangled `fn_XXXXXXXX` callees (unsplit, or owned by a registered unit whose header is
 * included above).  A linkage block keeps them out of the per-declaration checker; the owned ones
 * come from their owner's header. */
extern "C" {
void fn_800504D4(nw4r::math::MTX34* m);
void fn_80050850(nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_80050CA0(nw4r::math::VEC3* out, nw4r::math::MTX34* m, nw4r::math::VEC3* v);
void fn_80051378(nw4r::math::VEC3* out, nw4r::math::VEC3* a, nw4r::math::VEC3* b);
void fn_800513F0(nw4r::math::VEC3* v, f32 s);
void fn_80051EE0(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 s);
void fn_800834F0(void* p);
void fn_800B0B90(nw4r::math::VEC3* dst, nw4r::math::VEC3* src);
u8 fn_800CF208(void* model);
void fn_800DB6CC(nw4r::math::VEC3* pos);
void fn_800DB714(nw4r::math::VEC3* pos);
void fn_800DB75C(void* enemy, nw4r::math::VEC3* pos);
void fn_801B701C(_ENEMY_WORK* enemy);
void fn_80224884(_PLW* plw);
void fn_8027D7EC(_PLW* plw, u8 flag);
u32 fn_8029F564(void* self, u32 flag);
void fn_802B00AC(nw4r::math::VEC3* out, u8 area);
void fn_802BE038(void* self);
void fn_802D2B78(_ENEMY_WORK* enemy);
void fn_80331210(void* model);
void fn_803BEE04(void);
void* fn_800F8788(u32 pool);
void fn_800F886C(void* self);
void* fn_800F8914();
void fn_800F8A44(void* p, s32 mode);
u32 fn_800F92F4(void* self, u32 flag);
u32 fn_800F9380(_PLW* plw);
void fn_800F93D8(void* self, nw4r::ef::Effect** effects, s32 count, s32 mode, void* arg);
u8 fn_800F9D80(void* self);
void fn_800F9DF4(void* self, u8 a, u8 b);
void fn_800FAD90(_EFT* self);
void fn_800FADCC(_EFT* self);
void fn_800AA75C(void* dst);

/* The unit's own `.sdata`/`.data` pool, referenced by name (never emitted here). */
extern u8 lbl_807916C0[8];
extern u32 lbl_807916A0[];
extern u16 lbl_8059B638[];
extern u16 lbl_8059B670[];
extern f32 lbl_80796608;
extern f32 lbl_8079660C;
extern f32 lbl_80796610;
extern f32 lbl_80796614;
extern f32 lbl_80796644;
extern f32 lbl_80796640;
extern f32 lbl_80796648;
extern u8 lbl_8059B6A8[];
extern u8 lbl_8059B6B4[];
extern u8 lbl_8059B6CC[];
extern u8 lbl_8059B6E4[];
extern u8 lbl_8059B7C8[];
}

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800FB160(_EFT* self);
extern "C" void fn_800FC3A4(_EFT* self);
extern "C" void fn_800FC3E0(_EFT* self);
extern "C" void fn_800FC484(_EFT* self);
extern "C" void fn_800FC7EC(_EFT* self);
extern "C" void fn_800FCA34(_EFT* self);
extern "C" void fn_800FCA54(_EFT* self);

/* Writes a VEC3 into an MTX34's translation column: `m[0][3]`, `m[1][3]`, `m[2][3]`. */
extern "C" void fn_800FBB90(nw4r::math::MTX34* m, nw4r::math::VEC3* v)
{
    m->m[0][3] = v->x;
    m->m[1][3] = v->y;
    m->m[2][3] = v->z;
}

/* Bumps the effect's `byte_5` state index. */
extern "C" void fn_800FBBAC(_EFT* self)
{
    self->state_0x05++;
}

/* Retires the effect object: hands it back to the pool through the shared destructor. */
extern "C" void fn_800FBBBC(_EFT* self)
{
    fn_800F886C(self);
}

/* Copies a 12-byte rotation triple. */
extern "C" void fn_800FC0D4(_CP_VECTOR* dst, _CP_VECTOR* src)
{
    *dst = *src;
}

/* Dispatches to the two per-frame model handlers by the effect type. */
extern "C" void fn_800FC384(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s8)self->type_0x02 == 14) {
        fn_800FC3E0(self);
    } else {
        fn_800FC3A4(self);
    }
}

/* Retires the pooled effect of the work block and clears its count. */
extern "C" void fn_800FC3A4(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;

    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->effect_0x04, work->count);
    work->count = 0;
}

/* Releases the secondary effect's pool entry, retires the primary and clears the count. */
extern "C" void fn_800FC3E0(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;

    fn_800F8A44(&work->effect_0x08, 1);
    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->effect_0x04, work->count);
    work->count = 0;
}

/* The per-frame dispatcher: advance the state handler the effect's `byte_5` selects. */
extern "C" void fn_800FC428(_EFT* self)
{
    switch (self->state_0x05) {
    case 1:
        fn_800FCA34(self);
        break;
    case 2:
        fn_800FD29C(self);
        break;
    case 3:
        fn_800FD2AC(self);
        break;
    case 0:
        if ((u32)(self->type_0x02 - 18) <= 1 || (s8)self->type_0x02 == 14) {
            fn_800FC7EC(self);
        } else {
            fn_800FC484(self);
        }
        break;
    }
}

/* Dispatches to the two per-frame model handlers of the second half of the unit. */
extern "C" void fn_800FCA34(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s8)self->type_0x02 == 14) {
        fn_800FCED4(self);
    } else {
        fn_800FCA54(self);
    }
}

/* Builds a new effect of `type`: allocates it from the 32-entry pool, installs the dispatcher and the
 * pool-release handler, fills the model list from the type's descriptor table and clears the pose. */
extern "C" _EFT* fn_800FBD68(u32 type)
{
    _EFT* effect = (_EFT*)fn_800F8788(32);
    _EFT001_MODEL_WORK* work;
    u8* table = lbl_807916C0;
    s32 i;
    MHchar** slot;

    if (effect == NULL) {
        return NULL;
    }

    effect->type_0x02 = (u8)type;
    effect->dispatch_0x34 = fn_800FADCC;
    effect->release_0x40 = fn_800FAD90;

    work = (_EFT001_MODEL_WORK*)effect->work_0x38;
    work->count = table[type];

    for (i = 0, slot = work->models; i < work->count; i++, slot++) {
        *slot = (MHchar*)fn_800F8914();
        if (*slot == NULL) {
            fn_800F886C(effect);
            return NULL;
        }
    }

    effect->rot_0x24.x = 0;
    effect->rot_0x24.y = 0;
    effect->rot_0x24.z = 0;
    effect->field_0x10 = 0;
    effect->timer_0x0C = 0;
    fn_800F9DF4(effect, 1, 0);
    return effect;
}

/* Builds a model effect at `pos`: gated on the current area, allocates a 16-entry pool effect,
 * installs the two per-frame handlers, seeds the model list and the parameter scale. */
extern "C" _EFT* fn_800FC27C(nw4r::math::VEC3* pos, u32 type, u32 param, u32 area, f32 scale)
{
    if ((u8)area != (u8)get_now_areano()) {
        return NULL;
    }

    _EFT* effect = (_EFT*)fn_800F8788(16);
    if (effect == NULL) {
        return NULL;
    }

    effect->type_0x02 = (u8)type;
    effect->release_0x40 = fn_800FC384;
    effect->dispatch_0x34 = fn_800FC428;

    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)effect->work_0x38;
    work->count = 1;
    work->effect_0x08 = (nw4r::ef::Effect*)fn_800F8914();
    if (work->effect_0x08 == NULL) {
        fn_800F886C(effect);
        return NULL;
    }

    work->scale_0x0C = scale;
    effect->field_0x03 = 1;
    effect->timer_0x0C = 0;
    copyVec3(&effect->pos_0x18, pos);
    effect->rot_0x24.y = param;
    effect->area_0x44 = (u8)area;
    fn_800F9DF4(effect, 0, 4);
    return effect;
}

/* Wrapper around `fn_800FC27C` for a `_CP_VECTOR` rotation triple: its second word is the parameter
 * stored on the effect. */
void eft001_set_pos(nw4r::math::VEC3* pos, u32 a, _CP_VECTOR* v, u8 b, f32 c)
{
    if (fn_800FC27C(pos, a, v->y, b, c) == NULL) {
        return;
    }
}

/* Builds a model effect from a joint source: its +0x36 word and +0x0F area seed `fn_800FC27C`, and the
 * effect's `field_0x07` records whether the source owns the 0x800 flag. */
extern "C" void fn_800FC1DC(nw4r::math::VEC3* pos, u32 type, _EFT001_SRC* src)
{
    _EFT* effect = fn_800FC27C(pos, type, src->param_0x36, src->area_0x0F, lbl_80796644);

    if (effect != NULL) {
        effect->field_0x07 = (fn_8029F564(src, 2048) == 1) ? 1 : 0;
    }
}

/* Builds a model effect with a full parameter set: the area gate, the model builder, the effect's
 * `field_0x08`/`field_0x06` seed bytes, the rotation triple and the parameter scale. */
extern "C" void fn_800FC0F0(nw4r::math::VEC3* pos, u32 type, u32 field_08, u32 area, _CP_VECTOR* rot, f32 scale)
{
    if ((u8)area != (u8)get_now_areano()) {
        return;
    }

    _EFT* effect = (_EFT*)fn_800F8788(44);
    if (effect == NULL) {
        return;
    }

    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)effect->work_0x38;
    work->count = 1;
    effect->type_0x02 = (u8)type;
    effect->demo_flag_0x08 = (u8)field_08;
    effect->field_0x03 = 1;
    effect->timer_0x0C = 0;
    copyVec3(&effect->pos_0x18, pos);
    fn_800FC0D4(&effect->rot_0x24, rot);
    effect->area_0x44 = (u8)area;
    work->value_0x18 = scale;
    effect->field_0x07 = 4;
    effect->field_0x06 = 0;
    fn_800F9DF4(effect, 0, 4);
    effect->release_0x40 = fn_800FC384;
    effect->dispatch_0x34 = fn_800FC428;
}

/* The particle-manager walk callback: forwards the entry (offset by its 88-byte header) to the shared
 * particle release. */
extern "C" void fn_800FCEC8(void* entry, u32 index)
{
    (void)index;
    fn_800AA75C(&((_EFT001_PM_ENTRY*)entry)->field_0x58);
}

/* Walks the effect's particle-manager pool through `ForeachParticleManager`. */
extern "C" void fn_800FCEB0(nw4r::ef::Effect* self, u32 arg, bool flag)
{
    self->ForeachParticleManager(fn_800FCEC8, arg, flag);
}

/* Builds the pooled effect + model pair for a per-frame effect: allocates both from the type's id
 * tables, poses the model at the effect's position, offsets its animation clock by a random amount,
 * and seeds the two K-colours the effect family uses. */
extern "C" void fn_800FC7EC(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;
    MHchar* model;
    _GXColor color;

    self->state_0x05++;

    work->effect_0x04 = res_eft_create(lbl_8059B638[self->type_0x02], lbl_8059B670[self->type_0x02], 0);
    if (work->effect_0x04 == NULL) {
        fn_800FD2AC(self);
        return;
    }

    if (res_eft_model_create((MHchar*)work->effect_0x08, 82, 340) == NULL) {
        fn_800FD2AC(self);
        return;
    }

    model = (MHchar*)work->effect_0x08;
    SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
    copyVec3(&model->pos_0x04, &self->pos_0x18);
    fn_800FC0D4(&model->rot_0x54, &self->rot_0x24);

    model->field_0x2C -= 3641;
    model->field_0x2C += ran_suu(0) & 0x1FFF;
    model->field_0x30 = 0xF1C7;
    model->field_0x30 += ran_suu(0) & 0x1FFF;

    model->setVisibility(1, false);
    setVector3(&model->scale_0x1C, work->scale_0x0C, work->scale_0x0C, work->scale_0x0C);

    self->timer_0x0C = 5;
    self->flag_0x01 = 1;

    switch (self->type_0x02) {
    case 14:
        color.r = 0x66;
        color.g = 0xEE;
        color.b = 0xFF;
        color.a = 0xFF;
        break;
    case 18:
        color.r = 0xFF;
        color.g = 0;
        color.b = 0;
        color.a = 0xFF;
        {
            s32 i;
            for (i = 0; i < 2; i++) {
                model->setMatAlphaBlendMode(i, GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            }
        }
        break;
    case 19:
        color.r = 0xFF;
        color.g = 0xFF;
        color.b = 0xC8;
        color.a = 0xFF;
        break;
    }

    {
        s32 i;
        for (i = 0; i < 2; i++) {
            model->setTevKColor(i, GX_KCOLOR3, &color);
        }
    }

    self->field_0x06 = 0;
    fn_800FCA34(self);
}

/* Places the `index`-th model of a model effect on a joint whose ground height the player's state
 * selects (an airborne/landing y, or a traced ground hit).  Runs only for type 0. */
extern "C" void fn_800FBBC0(_EFT* self, u32 index, u32 flags)
{
    nw4r::math::VEC3 pos;
    _EFT001_PLW_VIEW* plw;
    MHchar* model;
    _EFT001_MODEL_WORK* work;

    VEC3_ctor(&pos);
    work = (_EFT001_MODEL_WORK*)self->work_0x38;
    plw = (_EFT001_PLW_VIEW*)self->source_0x30;

    if (self->type_0x02 != 0) {
        return;
    }

    model = &((_EFT001_PHYSICS*)plw->physics_0x13C)->joint_0x004;
    model->get_joint_wpos(flags, &pos);

    switch (fn_800CF208(model)) {
    case 2:
        pos.y = lbl_80796610 + plw->field_0x060;
        break;
    default:
        switch (plw->field_0x00A) {
        case 2:
            if (plw->field_0x00C <= 1 || (u16)(plw->field_0x00C - 5) <= 1) {
                pos.y = lbl_80796610 + plw->field_0x060;
            } else {
                pos.y += lbl_80796608;
                pos.y = lbl_80796610 + GetGroundHit(&pos, (u32)-6, self->area_0x44);
            }
            break;
        case 12:
            if ((u16)(plw->field_0x00C - 5) <= 1) {
                pos.y = lbl_80796610 + plw->field_0x060;
            } else {
                pos.y += lbl_80796608;
                pos.y = lbl_80796610 + GetGroundHit(&pos, (u32)-6, self->area_0x44);
            }
            break;
        default:
            pos.y += lbl_80796608;
            pos.y = lbl_80796610 + GetGroundHit(&pos, (u32)-6, self->area_0x44);
            break;
        }
        break;
    }

    copyVec3(&work->models[index]->pos_0x04, &pos);
}

/* The per-frame body of a model effect: builds its models on the first frame, then places and
 * recolours them from the enemy's pose (or the `type_0x02 == 2` variant).  `state_0x05` selects the
 * stage: 0 creates, 1 places on an enemy joint, 2 re-places after the model swap. */
extern "C" void fn_800FAE08(_EFT* self)
{
    _EFT001_MODEL_WORK* work = (_EFT001_MODEL_WORK*)self->work_0x38;
    s32 i;

    self->state_0x05++;

    switch (self->type_0x02) {
    case 0:
        for (i = 0; i < work->count; i++) {
            if (res_eft_model_create(work->models[i], 0, 24) == NULL) {
                fn_800FBBBC(self);
                return;
            }
        }
        for (i = 0; i < work->count; i++) {
            MHchar* model = work->models[i];
            model->setVisibility(1, true);
            model->setVisibility(2, false);
            setVector3(&model->scale_0x1C, lbl_80796608, lbl_80796608, lbl_80796608);
        }
        break;
    case 1:
        for (i = 0; i < work->count; i++) {
            if (res_eft_model_create(work->models[i], (u16)lbl_807916A0[i], 24) == NULL) {
                fn_800FBBBC(self);
                return;
            }
        }
        self->field_0x06 = ran_suu(0) & 0xF;

        {
            _EFT001_ENEMY* enemy = (_EFT001_ENEMY*)self->source_0x30;
            f32 scale;

            work->models[0]->setVisibility(1, false);
            work->models[0]->setVisibility(2, true);
            scale = fn_80135998(enemy);
            for (i = 0; i < work->count; i++) {
                MHchar* model = work->models[i];
                setVector3(&model->scale_0x1C, scale, scale, scale);
            }

            if (enemy->joint_0x204 == (u32)-1) {
                copyVec3(&self->pos_0x18, &enemy->pos_0x188);
            } else {
                get_joint_wpos_em((_ENEMY_WORK*)enemy, enemy->joint_0x204, &self->pos_0x18);
            }

            if ((((enemy->flags_0x228 & 2) != 0) ||
                 (enemy->state_0x1E2 == 2 &&
                  enemy->pos_0x188.y > (enemy->value_0x210 - lbl_8079660C * scale))) &&
                enemy->state_0x1E2 != 1 && enemy->state_0x1E2 != 3) {
                self->pos_0x18.y = lbl_80796610 + enemy->value_0x210;
            } else {
                self->pos_0x18.y = lbl_80796610 + enemy->value_0x20C;
            }
        }
        return;
    case 2:
        for (i = 0; i < work->count; i++) {
            if (res_eft_model_create(work->models[i], 0, 24) == NULL) {
                fn_800FBBBC(self);
                return;
            }
        }
        work->models[0]->setVisibility(1, false);
        work->models[0]->setVisibility(2, true);
        for (i = 0; i < work->count; i++) {
            MHchar* model = work->models[i];
            setVector3(&model->scale_0x1C, lbl_80796614, lbl_80796614, lbl_80796614);
        }
        break;
    default:
        fn_800FBBBC(self);
        return;
    }

    work->color_0x19[0] = 0xFF;
    work->color_0x19[1] = 0xFF;
    work->color_0x19[2] = 0xFF;
    work->color_0x19[3] = 0xDC;
    work->models[0]->setTevKColor(0, GX_KCOLOR0, (_GXColor*)work->color_0x19);
    fn_800FB160(self);
}

/* The per-frame body of the non-model (type 11..26) effect: advances the two-stage machine, moves
 * the pooled effects, and recolours them per type from the key tables. */
extern "C" void fn_800FCA54(_EFT* self)
{
    u8 buf[8];
    nw4r::math::VEC3 vec;
    _GXColor color;
    s32 live = 0;
    s32 i;
    _EFT001_EFFECT_WORK* work;

    fn_800834F0(buf);
    VEC3_ctor(&vec);
    work = (_EFT001_EFFECT_WORK*)self->work_0x38;

    if (self->field_0x06 == 0) {
        if (--self->timer_0x0C > 0) {
            return;
        }

        self->field_0x06++;

        switch (self->type_0x02) {
        case 7:
        case 8:
            fn_800DB608(2, &self->pos_0x18, work->flag_0x29);
            break;
        case 15:
            fn_800DB608(3, &self->pos_0x18, work->flag_0x29);
            break;
        case 17:
            fn_800DB608(4, &self->pos_0x18, work->flag_0x29);
            break;
        case 16:
            fn_800DB608(5, &self->pos_0x18, work->flag_0x29);
            break;
        case 20:
            fn_800DB608(6, &self->pos_0x18, work->flag_0x29);
            break;
        case 21:
            fn_800DB608(7, &self->pos_0x18, work->flag_0x29);
            break;
        case 11:
            switch (self->demo_flag_0x08) {
            case 0:
            case 2:
                fn_800DB6CC(&self->pos_0x18);
                break;
            case 1:
                fn_800DB714(&self->pos_0x18);
                break;
            }
            break;
        case 12:
            if (fn_800F92F4(self, 0) == 1) {
                fn_800DB75C(self->source_0x30, &self->pos_0x18);
            }
            break;
        }
        return;
    }

    if (self->type_0x02 == 7) {
        ((nw4r::math::VEC3*)buf)->x = lbl_80796640;
        ((nw4r::math::VEC3*)buf)->y = lbl_80796640;
        fn_800FCEB0(work->effect_0x04, (u32)buf, true);
    }

    if (self->type_0x02 <= 5 || (u8)(self->type_0x02 - 9) <= 1 ||
        self->type_0x02 == 22 || self->type_0x02 == 26) {
        change_paramscale_eff(work->effect_0x04, work->scale_0x0C);
    } else if (self->type_0x02 == 25) {
        setVector3(&vec, work->scale_0x0C, work->scale_0x0C, lbl_80796648 * work->scale_0x0C);
        change_paramscale_eff_vec3(work->effect_0x04, &vec);
    }

    for (i = 0; i < work->count; i++) {
        if (effect_move(((nw4r::ef::Effect**)&work->effect_0x04)[i]) == 0) {
            live = (u8)(live + 1);
        }
    }

    if (live >= work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    self->field_0x10++;

    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2:
    case 24:
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        break;
    case 7:
    case 8:
        if (fn_800F9D80(self) == 1) {
            work->color_0x14[0] = 100;
            work->color_0x14[1] = 191;
            work->color_0x14[2] = 255;
            work->color_0x14[3] = 255;
            color = *(_GXColor*)work->color_0x14;
            change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        }
        break;
    case 11:
        if (work->flag_0x28 == 0) {
            eftGetKeyRGB(lbl_8059B6E4 + self->demo_flag_0x08 * 4, self->field_0x10, &work->color_0x14[0],
                         &work->color_0x14[1], &work->color_0x14[2]);
        } else {
            eftGetKeyRGB(lbl_8059B6B4, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                         &work->color_0x14[2]);
        }
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        break;
    case 20:
        if (work->flag_0x28 == 0) {
            eftGetKeyRGB(lbl_8059B6A8, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                         &work->color_0x14[2]);
        } else {
            color.r = 255;
            color.g = 175;
            color.b = 175;
            color.a = 255;
            change_color_eff(work->effect_0x04, &self->pos_0x18, color);
            eftGetKeyRGB(lbl_8059B6B4, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                         &work->color_0x14[2]);
        }
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x08, &self->pos_0x18, color);
        break;
    case 21:
        eftGetKeyRGB(lbl_8059B6CC, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                     &work->color_0x14[2]);
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x08, &self->pos_0x18, color);
        break;
    case 25:
    case 26:
        eftGetKeyRGB(lbl_8059B7C8 + self->demo_flag_0x08 * 4, self->field_0x10, &work->color_0x14[0],
                     &work->color_0x14[1], &work->color_0x14[2]);
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        break;
    }

    fn_800F93D8(self, (nw4r::ef::Effect**)&work->effect_0x04, 1, work->count, NULL);
}

/* ---------------------------------------------------------------------------------------------------
 * still stubbed - the residual (see the unit report)
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800FB160(_EFT* self) {}
extern "C" void fn_800FC484(_EFT* self) {}
extern "C" void fn_800FBE64(void) {}
