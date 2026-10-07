/* ef/eft001.cpp - the head of the eft001 effect family: the model-placement half of a 0x48-byte `_EFT` record whose
 *   +0x38 pool block carries the pooled effect and its `MHchar` model(s), re-posed on an enemy joint or the player.
 * RANGE. .text 0x800FAE08-0x800FBE64 (7 functions); extab 0x8000BD6C-0x8000BD8C, extabindex 0x80025B0C-0x80025B3C,
 *   .data 0x8059B5F8-0x8059B638, .sdata 0x80791698-0x807916C8, .sdata2 0x80796608-0x80796640.
 * NAMES. `eft001` is the family of the runtime dump's `eft001_set_pos` (in `ef/eft002.cpp`); the map has only `fn_`
 *   stems here, so the definitions are `extern "C"`.
 *   GUESS (from the body): `mtx34_set_trans` (0x800FBB90) writes a vector into a matrix's translation column.
 * RESIDUALS. 1 row unwritten (an empty stub): 0x800FB160-0x800FBB90 (`fn_800FB160`).
 *   3 partial rows:
 *  - `fn_800FAE08`: ours fuses `fnmsubs` and `rlwinm.` where retail keeps `fmuls` + `fsubs` and the compare, keeps
 *    `+0x04`'s pointer in a register where retail reloads it, and addresses `lbl_807916A0` with `lis`/`addi` where
 *    retail uses `@sda21`;
 *  - `fn_800FBBC0`: ours compares the type signed (`cmpwi`) and drops the `addis` of the `u16` key;
 *  - `fn_800FBD68`: ours drops the `clrlwi` narrowing of the `u8` index and keeps the hook addresses in the stored
 *    register.
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x620 of 0x105C), extab (0x18 of 0x20) and
 *   extabindex (0x24 of 0x30) short of the claim and differing.
 */

#include "ef/fn_800FAD90.h" /* fn_800FAD90 (rule 2: the owner's header) */
#include "ef/fn_800FADCC.h" /* fn_800FADCC (rule 2: the owner's header) */
#include "enemy/fn_80135998.h" /* fn_80135998 (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/fn_800D7F54.h"
#include "ef/eft002.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800FAD90_c1 ((void (*)(_EFT*))fn_800FAD90)
#define fn_800FADCC_c1 ((void (*)(_EFT*))fn_800FADCC)

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

s32 ran_suu(s32 range);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);

void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);

void get_joint_wpos_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::VEC3* out);
f32 GetGroundHit(nw4r::math::VEC3* pos, u32 ground, u8 flag);

extern "C" {
u8 GameMode_ck(void* model);

void* eft_res_slot_get(u32 pool);
void eft_res_slot_release(void* self);
void* eft_res_model_get();

/* untyped: an opaque handle passed through (the callee only hands the pointer on) */
void eft_state_flags_set(void* self, u8 a, u8 b);

/* The unit's own `.sdata`/`.data` pool, referenced by name (never emitted here). */
extern u8 lbl_807916C0[8];
extern u32 lbl_807916A0[];

extern f32 lbl_80796608;
extern f32 lbl_8079660C;
extern f32 lbl_80796610;
extern f32 lbl_80796614;
}

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_800FB160(_EFT* self);

/* Writes a VEC3 into an MTX34's translation column: `m[0][3]`, `m[1][3]`, `m[2][3]`. */
extern "C" void mtx34_set_trans(nw4r::math::MTX34* m, nw4r::math::VEC3* v)
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
    eft_res_slot_release(self);
}

/* Builds a new effect of `type`: allocates it from the 32-entry pool, installs the dispatcher and the
 * pool-release handler, fills the model list from the type's descriptor table and clears the pose. */
extern "C" _EFT* fn_800FBD68(u32 type)
{
    _EFT* effect = (_EFT*)eft_res_slot_get(32);
    _EFT001_MODEL_WORK* work;
    u8* table = lbl_807916C0;
    s32 i;
    MHchar** slot;

    if (effect == NULL) {
        return NULL;
    }

    effect->type_0x02 = (u8)type;
    effect->dispatch_0x34 = fn_800FADCC_c1;
    effect->release_0x40 = fn_800FAD90_c1;

    work = (_EFT001_MODEL_WORK*)effect->work_0x38;
    work->count = table[type];

    for (i = 0, slot = work->models; i < work->count; i++, slot++) {
        *slot = (MHchar*)eft_res_model_get();
        if (*slot == NULL) {
            eft_res_slot_release(effect);
            return NULL;
        }
    }

    effect->rot_0x24.x = 0;
    effect->rot_0x24.y = 0;
    effect->rot_0x24.z = 0;
    effect->field_0x10 = 0;
    effect->timer_0x0C = 0;
    eft_state_flags_set(effect, 1, 0);
    return effect;
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

    switch (GameMode_ck(model)) {
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

/* Builds the model effect's models on the first frame, then places and recolours them from the enemy's pose;
 * `state_0x05` 0 creates, 1 places on an enemy joint, 2 re-places after the model swap. */
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

/* Not written yet (an empty stub). */
extern "C" void fn_800FB160(_EFT* self) {}
