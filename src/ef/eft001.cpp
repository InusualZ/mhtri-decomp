/* ef/eft001.cpp - the head of the eft001 effect family: the model-placement half of a 0x48-byte `_EFT` record whose
 *   +0x38 pool block carries the pooled effect and its `MHchar` model(s), re-posed on an enemy joint or the player.
 * RANGE. .text 0x800FAE08-0x800FBE64 (7 functions); extab 0x8000BD6C-0x8000BD8C, extabindex 0x80025B0C-0x80025B3C,
 *   .data 0x8059B5F8-0x8059B638, .sdata 0x80791698-0x807916C8, .sdata2 0x80796608-0x80796640.
 * NAMES. `eft001` is the family of the runtime dump's `eft001_set_pos` (in `ef/eft002.cpp`); the map has only `fn_`
 *   stems here, so the definitions are `extern "C"`.
 *   GUESS (from the body): `mtx34_set_trans` (0x800FBB90) writes a vector into a matrix's translation column.
 *   GUESS (from what they hold and who reads them): the `eft001_*` tables (0x8059B5F8..0x807916C0) and the callees
 *   `get_now_area_pos`, `pl_eft_draw_ck`, `pl_act_no_eft_ck`, `em_eft_hide_ck`, `em034_eft_ck`, `get_camera_rot_y`,
 *   `ai_npc_flag_ck`.
 * FLAGS. `cflags_main`; file-scope `#pragma pool_data off`, `peephole off` and `fp_contract off` (retail keeps one
 *   `lis`/`addi` per table, the unfused `andi.`+`cmpwi` and `clrlwi`+`cmpwi` compares, and `fmuls`+`fsubs`).
 * RESIDUALS. None: every row matches and flipcheck is READY.
 * SHAPES. `fn_800FB160`'s airborne/grounded split is a `do { ... break; ... } while (0)`: the airborne path `break`s
 *   past the grounded block to the shared tail, the timer/flag exits fall into it (retail's block order).
 *   Its locals are declared at the top in one block; the order `plw, i, glow, cycle, enemy, alpha_keys, scale_keys`
 *   is the one that gives retail's registers.  `if (a == 0 || b() == 0) return;` keeps retail's `bne`+`b`.
 *   `(u16)(x + (u16)-52) <= 2` gives retail's `addis`/`addi` range test of a `u16` field.
 *   `GameMode_ck` takes no argument; `fn_800FBD68` takes its type as `u8`.
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
#include "ef/eft001.h"
#include "ef/eft002.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "fn_8004CAD8.h" /* the vector helpers, getKeyData, mtx34_identity (rule 2) */
#include "vec3_scale.h"
#include "rotLocalMatX.h"
#include "ef/ef_postfield.h" /* vec3_sub_assign (rule 2) */
#include "ef/eft_res_spawn_gate_ck.h"
#include "ef/GameMode_ck.h"
#include "ef/eft_res_models_spawn.h"
#include "ef/eft_stage_spawn_flag.h"
#include "Pl/Pl_master_ck.h"
#include "enemy/fn_8012BDF4.h" /* em_work_die_ck (rule 2) */
#include "enemy/fn_8012EC74.h" /* get_joint_wmat_em (rule 2) */
#include "ai/ai_npc.h"
#include "unsplit/unknown.h" /* get_stg_eft_col, eftGetKeyAlpha */
#include "enemy/em_common.h"   /* em_eft_hide_ck */
#include "enemy/em034_prog.h"   /* em034_eft_ck */
#include "camera/camera_main.h"   /* get_camera_rot_y */
#include "stage/stg_w.h" /* get_now_area_pos (rule 2) */
#include "lobby/lb_equip_page.h" /* pl_eft_draw_ck (rule 2) */
#include "Pl/pl_act_no_eft_ck.h" /* pl_act_no_eft_ck (rule 2: a leaf header) */
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
    /* +0x0C */ nw4r::math::VEC3 area_pos_0x0C; /* the current area's reference position, refreshed each frame */
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
 * spells +0x0C `unk00C`, which rule 7 forbids in this file).  size: 0x5A8 - lower bound. */
struct _EFT001_PLW_VIEW {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 visible_0x001;    /* copied into the effect's own visibility flag */
    /* +0x002 */ u8 unused_0x002[0x07];
    /* +0x009 */ u8 kind_0x009;       /* 3 hides the effect */
    /* +0x00A */ u8 field_0x00A;      /* selects the pose branch (2 = airborne variant, 12 = grounded) */
    /* +0x00B */ u8 unused_0x00B;
    /* +0x00C */ u16 field_0x00C;     /* the ground/step state the pose branch gates on */
    /* +0x00E */ u8 unused_0x00E[0x08];
    /* +0x016 */ u8 area_0x016;       /* the area the player stands in */
    /* +0x017 */ u8 unused_0x017[0x25];
    /* +0x03C */ nw4r::math::VEC3 pos_0x03C; /* the player's position the effect aims away from */
    /* +0x048 */ u8 unused_0x048[0x18];
    /* +0x060 */ f32 field_0x060;     /* the player's base y the effect sits on */
    /* +0x064 */ u8 unused_0x064[0xD8];
    /* +0x13C */ void* physics_0x13C; /* the joint holder the effect is placed on */
    /* +0x140 */ u8 unused_0x140[0x5A4 - 0x140];
    /* +0x5A4 */ u16 flags_0x5A4;     /* bits 0x5000 hide the effect */
};

/* The enemy work record `fn_800FAE08` drives: only the offsets the state machine reads are named
 * (`_ENEMY_WORK`'s canonical view names fewer of them).  A unit-local view of the same record, cast
 * from `_EFT::source_0x30` where the effect is attached to an enemy.  size: 0x8D4 - lower bound. */
struct _EFT001_ENEMY {
    /* +0x000 */ u8 unused_0x000;
    /* +0x001 */ u8 visible_0x001;     /* copied into the effect's own visibility flag */
    /* +0x002 */ u8 unused_0x002;
    /* +0x003 */ u8 kind_0x003;        /* the monster kind; 36..39 take the glow branch */
    /* +0x004 */ u8 unused_0x004[0x188 - 0x004];
    /* +0x188 */ nw4r::math::VEC3 pos_0x188;
    /* +0x194 */ u8 unused_0x194[0x1BC - 0x194];
    /* +0x1BC */ u32 rot_x_0x1BC;      /* the body's pitch */
    /* +0x1C0 */ u32 rot_y_0x1C0;      /* the body's yaw */
    /* +0x1C4 */ u8 unused_0x1C4[0x04];
    /* +0x1C8 */ u32 flags_0x1C8;      /* bit 0x400 forces the grounded glow */
    /* +0x1CC */ u8 unused_0x1CC[0x1D4 - 0x1CC];
    /* +0x1D4 */ f32 alpha_0x1D4;      /* the body's alpha ratio the K-colour alpha is scaled by */
    /* +0x1D8 */ u8 unused_0x1D8[0x1E1 - 0x1D8];
    /* +0x1E1 */ u8 area_0x1E1;        /* the area the enemy is in */
    /* +0x1E2 */ u8 state_0x1E2;       /* the action sub-state the pose gate reads */
    /* +0x1E3 */ u8 unused_0x1E3[0x02];
    /* +0x1E5 */ u8 action_0x1E5;      /* 11 with +0x8D2 set switches to the alternate key set */
    /* +0x1E6 */ u8 unused_0x1E6[0x204 - 0x1E6];
    /* +0x204 */ u32 joint_0x204;      /* the joint id `get_joint_wpos_em` places on (0xFFFFFFFF = the root) */
    /* +0x208 */ u8 unused_0x208[0x20C - 0x208];
    /* +0x20C */ f32 value_0x20C;      /* the ground-drop y for a grounded pose */
    /* +0x210 */ f32 value_0x210;      /* the base y for an airborne pose */
    /* +0x214 */ u8 unused_0x214[0x228 - 0x214];
    /* +0x228 */ u16 flags_0x228;
    /* +0x22A */ u8 unused_0x22A[0x8D2 - 0x22A];
    /* +0x8D2 */ u8 field_0x8D2;       /* with action 11, selects the alternate key set */
};

s32 ran_suu(s32 range);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);

void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);

void get_joint_wpos_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::VEC3* out);
f32 GetGroundHit(nw4r::math::VEC3* pos, u32 ground, u8 flag);

extern "C" {

void* eft_res_slot_get(u32 pool);
void eft_res_slot_release(void* self);
void* eft_res_model_get();

/* untyped: an opaque handle passed through (the callee only hands the pointer on) */
void eft_state_flags_set(void* self, u8 a, u8 b);
}

#pragma pool_data off
#pragma peephole off
#pragma fp_contract off

/* STOPGAP-BEGIN(ef-game3#8) */
s32 calcVecAng3(nw4r::math::VEC3* v);
/* STOPGAP-END(ef-game3#8) */

/* The glow's offset from the enemy's joint (y, z); `.sdata` words, not pooled constants. */
f32 eft001_glow_offset_y = 50.0f;
f32 eft001_glow_offset_z = 40.0f;
/* The model id each model of a type-1 effect is created with. */
u32 eft001_model_ids[2] = {0, 0x4D};

/* The scale keys (frame, value pairs, -1 ends) the airborne glow grows by, and the alternate set. */
f32 eft001_scale_keys[6] = {0.0f, 0.5f, 20.0f, 1.3f, -1.0f, 1.3f};
f32 eft001_scale_keys_alt[6] = {0.0f, 0.8f, 40.0f, 1.4f, -1.0f, 1.4f};
/* The alpha keys (frame, alpha pairs, 0xFF ends) of the airborne glow, and the alternate set. */
u8 eft001_alpha_keys[16] = {0x00, 0x00, 0x06, 0xDF, 0x10, 0x23, 0x14, 0x00, 0xFF};
u8 eft001_alpha_keys_alt[8] = {0x00, 0x00, 0x10, 0x9F, 0x28, 0x00, 0xFF};
/* The airborne glow's cycle length in frames, per key set. */
s32 eft001_cycle_frames[2] = {24, 50};
/* The alpha keys the grounded glow fades in by. */
u8 eft001_fade_alpha_keys[8] = {0x00, 0x00, 0x0A, 0xDC, 0xFF, 0xDC};
/* The number of models each effect type carries. */
u8 eft001_model_counts[8] = {2, 2, 1};

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */
extern "C" void fn_800FB160(_EFT* self);
extern "C" void fn_800FBBC0(_EFT* self, u32 index, u32 flags);
extern "C" void fn_800FBBBC(_EFT* self);

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
            work->models[i]->setVisibility(1, true);
            work->models[i]->setVisibility(2, false);
            setVector3(&work->models[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        }
        break;
    case 1:
        for (i = 0; i < work->count; i++) {
            if (res_eft_model_create(work->models[i], (u16)eft001_model_ids[i], 24) == NULL) {
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
                  enemy->pos_0x188.y > (enemy->value_0x210 - 200.0f * scale))) &&
                enemy->state_0x1E2 != 1 && enemy->state_0x1E2 != 3) {
                self->pos_0x18.y = 5.0f + enemy->value_0x210;
            } else {
                self->pos_0x18.y = 5.0f + enemy->value_0x20C;
            }
        }
        break;
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
            setVector3(&model->scale_0x1C, 0.15f, 0.15f, 0.15f);
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

/* Places the effect's models each frame on its owner: on the player (type 0) between two joints, aimed away from
 * the player; on an enemy (type 1) as a body glow keyed by the enemy's pose; on an NPC (type 2) at its head joint. */
extern "C" void fn_800FB160(_EFT* self)
{
    nw4r::math::VEC3 dir;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 offset;
    nw4r::math::VEC3 spawn_pos;
    u8 color[4];
    _EFT001_PLW_VIEW* plw;
    s32 i;
    u32 glow;
    s32 cycle;
    _EFT001_ENEMY* enemy;
    u8* alpha_keys;
    f32* scale_keys;
    f32 scale;
    f32 size;
    f32 rate;
    _AINPC_W* npc;

    VEC3_ctor(&dir);
    MTX34_ctor(&mtx);
    VEC3_ctor(&offset);
    VEC3_ctor(&spawn_pos);
    _EFT001_MODEL_WORK* work = (_EFT001_MODEL_WORK*)self->work_0x38;
    {
        nw4r::math::VEC3 area_pos;
        get_now_area_pos(&area_pos, self->area_0x44);
        copyVec3(&work->area_pos_0x0C, &area_pos);
    }

    switch (self->type_0x02) {
    case 0: {

        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        plw = (_EFT001_PLW_VIEW*)self->source_0x30;
        self->flag_0x01 = plw->visible_0x001;
        self->area_0x44 = plw->area_0x016;
        if (plw->kind_0x009 == 3 || pl_act_no_eft_ck((_PLW*)plw, 0) == 1 || (plw->flags_0x5A4 & 0x5000) != 0) {
            self->flag_0x01 = 0;
        } else if (plw->field_0x00A == 6 && (u16)(plw->field_0x00C + (u16)-52) <= 2) {
            self->flag_0x01 = 0;
        }
        if (self->flag_0x01 == 0 || pl_eft_draw_ck((_PLW*)plw) == 0) {
            return;
        }
        fn_800FBBC0(self, 0, 20);
        fn_800FBBC0(self, 1, 17);
        {
            nw4r::math::VEC3 mid;
            nw4r::math::VEC3 sum;
            addVec3(&sum, &work->models[0]->pos_0x04, &work->models[1]->pos_0x04);
            vec3_scale(&mid, &sum, 0.5f);
            copyVec3(&self->pos_0x18, &mid);
        }
        {
            nw4r::math::VEC3 diff;
            subVec3(&diff, &work->area_pos_0x0C, &plw->pos_0x03C);
            copyVec3(&dir, &diff);
        }
        vec3_normalize_into(&dir, &dir);
        self->rot_0x24.y = calcVecAng3(&dir);
        for (i = 0; i < work->count; i++) {
            work->models[i]->field_0x28 = 0;
            work->models[i]->field_0x2C = self->rot_0x24.y;
            work->models[i]->field_0x30 = 0;
            work->models[i]->move(0);
        }
        if (Pl_master_ck((_PLW*)plw) == 1) {
            eft_res_models_spawn(self, (void**)work->models, eft_stage_spawn_flag((_PLW*)plw) | 2, work->count, NULL);
        }
        return;
    }
    case 1: {
        enemy = (_EFT001_ENEMY*)self->source_0x30;
        if (em_work_die_ck((_ENEMY_WORK*)enemy) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        if (em_eft_hide_ck((_ENEMY_WORK*)enemy) == 1) {
            self->flag_0x01 = 0;
        } else {
            self->flag_0x01 = enemy->visible_0x001;
        }
        self->area_0x44 = enemy->area_0x1E1;
        if ((u32)(enemy->kind_0x003 - 36) <= 3) {
            if (em034_eft_ck((_ENEMY_WORK*)enemy) != 0) {
                glow = 1;
            } else {
                glow = 0;
            }
        } else {
            glow = 0;
        }
        if (glow == 1) {
            color[0] = work->color_0x19[0];
            color[1] = work->color_0x19[1];
            color[2] = work->color_0x19[2];
            color[3] = work->color_0x19[3];
            color[3] = color[3] * enemy->alpha_0x1D4;
            work->models[0]->setTevKColor(0, GX_KCOLOR0, (_GXColor*)color);
            get_joint_wmat_em((_ENEMY_WORK*)enemy, enemy->joint_0x204, &mtx);
            setVector3(&offset, 0.0f, eft001_glow_offset_y, eft001_glow_offset_z);
            mulVecMat(&offset, &mtx);
            copyVec3(&self->pos_0x18, &enemy->pos_0x188);
            vec3_sub_assign((Vec*)&self->pos_0x18, (Vec*)&offset);
            copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
            copyVec3(&spawn_pos, &self->pos_0x18);
            self->rot_0x24.y = enemy->rot_y_0x1C0;
            self->rot_0x24.x = enemy->rot_x_0x1BC;
            mtx34_identity(&mtx);
            rotLocalMatY(self->rot_0x24.y, &mtx);
            rotLocalMatX(self->rot_0x24.x, &mtx);
            mtx34_set_trans(&mtx, &self->pos_0x18);
            scale = fn_80135998(enemy);
            setVector3(&work->models[0]->scale_0x1C, 1.65f * scale, scale, 2.91f * scale);
            work->models[0]->move2(&mtx, 0);
            return;
        }

        do {
            if ((enemy->flags_0x1C8 & 0x400) == 0) {
                if (self->field_0x07 == 1) {
                    scale_keys = eft001_scale_keys_alt;
                    alpha_keys = eft001_alpha_keys_alt;
                    cycle = eft001_cycle_frames[1];
                } else {
                    scale_keys = eft001_scale_keys;
                    alpha_keys = eft001_alpha_keys;
                    cycle = eft001_cycle_frames[0];
                }
                scale = fn_80135998(enemy);
                if (self->field_0x06 != 0) {
                    self->field_0x06--;
                }
                if ((((enemy->flags_0x228 & 2) != 0) ||
                     (enemy->state_0x1E2 == 2 && enemy->pos_0x188.y > (enemy->value_0x210 - 200.0f * scale))) &&
                    enemy->state_0x1E2 != 1 && enemy->state_0x1E2 != 3) {
                    if (work->timer_0x18 > 0) {
                        work->timer_0x18--;
                    } else {
                        u32 stage_color;

                        if (self->field_0x06 != 0) {
                            return;
                        }
            self->timer_0x0C = 0;
            self->field_0x10++;
            scale = fn_80135998(enemy);
            if (scale > 0.5f) {
                rate = 1.0f + (scale - 0.5f);
            } else {
                rate = 1.0f;
            }
            if (self->field_0x10 > (s32)(cycle * rate)) {
                if (enemy->joint_0x204 == (u32)-1) {
                    copyVec3(&self->pos_0x18, &enemy->pos_0x188);
                } else {
                    get_joint_wpos_em((_ENEMY_WORK*)enemy, enemy->joint_0x204, &self->pos_0x18);
                }
                self->pos_0x18.y = 5.0f + enemy->value_0x210;
                self->field_0x10 = 0;
                self->rot_0x24.y = (u16)ran_suu(0);
                if (enemy->action_0x1E5 == 11 && enemy->field_0x8D2 != 0) {
                    self->field_0x07 = 1;
                } else {
                    self->field_0x07 = 0;
                }
            }
            size = getKeyData(scale_keys, self->field_0x10 * (1.0f / rate));
            size *= fn_80135998(enemy);
            setVector3(&work->models[1]->scale_0x1C, size, size, size);
            stage_color = get_stg_eft_col(self->area_0x44, 1);
            color[0] = (stage_color & 0xFF000000) >> 24;
            color[1] = (stage_color & 0xFF0000) >> 16;
            color[2] = (stage_color & 0xFF00) >> 8;
            color[3] = eftGetKeyAlpha(alpha_keys, (s32)(self->field_0x10 * (1.0f / rate)));
            color[3] = color[3] * enemy->alpha_0x1D4;
            work->models[1]->setTevKColor(0, GX_KCOLOR3, (_GXColor*)color);
            self->demo_flag_0x08 = 1;
            break;
                    }
                } else {
                    work->timer_0x18 = 10;
                }
            }

            scale = fn_80135998(enemy);
            if (scale > 0.5f) {
                rate = 1.0f + (scale - 0.5f);
            } else {
                rate = 1.0f;
            }
            self->field_0x10 = 24.0f * rate;
            if (enemy->joint_0x204 == (u32)-1) {
                copyVec3(&self->pos_0x18, &enemy->pos_0x188);
            } else {
                get_joint_wpos_em((_ENEMY_WORK*)enemy, enemy->joint_0x204, &self->pos_0x18);
            }
            if (++self->timer_0x0C > 10) {
                self->timer_0x0C = 10;
            }
            color[0] = work->color_0x19[0];
            color[1] = work->color_0x19[1];
            color[2] = work->color_0x19[2];
            color[3] = eftGetKeyAlpha(eft001_fade_alpha_keys, self->timer_0x0C);
            color[3] = color[3] * enemy->alpha_0x1D4;
            work->models[0]->setTevKColor(0, GX_KCOLOR0, (_GXColor*)color);
            self->pos_0x18.y = 5.0f + enemy->value_0x20C;
            size = fn_80135998(enemy);
            setVector3(&work->models[0]->scale_0x1C, size, size, size);
            self->rot_0x24.y = enemy->rot_y_0x1C0;
            self->demo_flag_0x08 = 0;
        } while (0);

        copyVec3(&work->models[self->demo_flag_0x08]->pos_0x04, &self->pos_0x18);
        setVector3(&offset, 0.0f, 0.0f, 350.0f);
        vec3_scale_in_place(&offset, size);
        rotVecY(&offset, (u16)get_camera_rot_y());
        {
            nw4r::math::VEC3 sum;
            addVec3(&sum, &self->pos_0x18, &offset);
            copyVec3(&spawn_pos, &sum);
        }
        work->models[self->demo_flag_0x08]->field_0x28 = 0;
        work->models[self->demo_flag_0x08]->field_0x2C = self->rot_0x24.y;
        work->models[self->demo_flag_0x08]->field_0x30 = 0;
        work->models[self->demo_flag_0x08]->move(0);
        eft_res_models_spawn(self, (void**)&work->models[self->demo_flag_0x08], 2, 1, &spawn_pos);
        return;
    }
    case 2: {

        if (eft_res_spawn_gate_ck(self, 2) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        npc = (_AINPC_W*)self->source_0x30;
        if (ai_npc_flag_ck(npc, 2) != 0 || (npc->field_0x324 & 0x4000) != 0) {
            self->flag_0x01 = 0;
        } else {
            self->flag_0x01 = npc->in_area;
        }
        self->area_0x44 = npc->area;
        npc->model.get_joint_wpos(3, &self->pos_0x18);
        self->pos_0x18.y = 5.0f + npc->field_0x19C;
        copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
        work->models[0]->move(0);
        eft_res_models_spawn(self, (void**)work->models, 2, work->count, NULL);
        return;
    }
    default:
        fn_800FBBBC(self);
        break;
    }
}

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

/* Places the `index`-th model of a model effect on a joint whose ground height the player's state
 * selects (an airborne/landing y, or a traced ground hit).  Runs only for type 0. */
extern "C" void fn_800FBBC0(_EFT* self, u32 index, u32 flags)
{
    nw4r::math::VEC3 pos;
    _EFT001_MODEL_WORK* work;
    _EFT001_PLW_VIEW* plw;

    VEC3_ctor(&pos);
    work = (_EFT001_MODEL_WORK*)self->work_0x38;
    plw = (_EFT001_PLW_VIEW*)self->source_0x30;

    if (self->type_0x02 != 0) {
        return;
    }

    ((_EFT001_PHYSICS*)plw->physics_0x13C)->joint_0x004.get_joint_wpos(flags, &pos);

    if (GameMode_ck() == 2) {
        pos.y = 5.0f + plw->field_0x060;
    } else {
        switch (plw->field_0x00A) {
        case 2:
            if (plw->field_0x00C <= 1 || (u16)(plw->field_0x00C + (u16)-5) <= 1) {
                pos.y = 5.0f + plw->field_0x060;
            } else {
                pos.y += 1.0f;
                pos.y = 5.0f + GetGroundHit(&pos, (u32)-6, self->area_0x44);
            }
            break;
        case 12:
            if ((u16)(plw->field_0x00C + (u16)-5) <= 1) {
                pos.y = 5.0f + plw->field_0x060;
            } else {
                pos.y += 1.0f;
                pos.y = 5.0f + GetGroundHit(&pos, (u32)-6, self->area_0x44);
            }
            break;
        default:
            pos.y += 1.0f;
            pos.y = 5.0f + GetGroundHit(&pos, (u32)-6, self->area_0x44);
            break;
        }
    }

    copyVec3(&work->models[index]->pos_0x04, &pos);
}

/* Builds a new effect of `type`: allocates it from the 32-entry pool, installs the dispatcher and the
 * pool-release handler, fills the model list from the type's descriptor table and clears the pose. */
extern "C" _EFT* fn_800FBD68(u8 type)
{
    s32 i;
    _EFT* effect;
    _EFT001_MODEL_WORK* work;

    effect = (_EFT*)eft_res_slot_get(32);
    if (effect == NULL) {
        return NULL;
    }

    effect->type_0x02 = type;
    effect->dispatch_0x34 = fn_800FADCC_c1;
    effect->release_0x40 = fn_800FAD90_c1;

    work = (_EFT001_MODEL_WORK*)effect->work_0x38;
    work->count = eft001_model_counts[type];

    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
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
