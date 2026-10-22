/* ef/eft029_fx.cpp - the eft029 player effect family (the setters `eft029_set_scale`/`eft029_set_kaihou`, the record
 *   builder, the release and state dispatchers, the per-type start and move handlers) and the eft030 stage-model
 *   family after it (the builder, release, dispatcher and its states).
 * RANGE. .text 0x8011AD58-0x8011D448 (27 functions); extab 0x8000C5A4-0x8000C63C, extabindex 0x80026760-0x80026844,
 *   .data 0x805A07F0-0x805A0C10, .sdata 0x80791978-0x807919C8, .sdata2 0x80796B50-0x80796C08.  The range is two TUs:
 *   eft029 is .text 0x8011AD58-0x8011CC24 (.data to 0x805A0B58, .sdata2 to 0x80796BC4) and eft030 the rest (its pool
 *   repeats eft029's 0.0f and int-to-float magic, which one pool cannot hold twice) - a seam request is filed.
 * FLAGS. `cflags_main`; `#pragma peephole off`, `#pragma optimization_level 4` (the switch range tests `subi`/`cmplwi`)
 *   and `#pragma pool_data off` over every body; `#pragma fp_contract off` for `eft030_move`.
 * NAMES. `eft029_set_scale` and `eft029_set_kaihou` are the runtime dump's own names.  GUESS (from the bodies): the
 *   rest of the functions (`eft029_set_half`, `eft029_create`, `eft029_release[_effects|_model]`, `eft029_dispatch`,
 *   the static `eft029_start`, `eft029_start_effects|model`, `eft029_move[_charge|_model_keys|_model_swing|_effects|
 *   _charge2|_hold]`, `eft029_state_next`, `eft029_free`, `eft029_spawn_ck`, `eft030_set|release|dispatch|start|move|
 *   state_next|free`) and every data name.  The map renames and the split of `eft029_dispatch` at +0x3C are requests.
 * RESIDUALS. `eft029_move_charge2`: retail's act-number decision tree compares with `cmplwi`; every `switch` spelling
 *   MWCC 1.3 lowers to `cmpwi` (probed: u16/u32/unsigned long/enum operands), and the shared fail block sits after the
 *   colour-scale call.  `.data` (jump tables after both families' tables) and `.sdata2` (one merged pool) differ until
 *   the seam is cut.
 * SHAPES. The out-pointer vector helpers are called through by-value spellings (`VEC3_SUB`, `VEC3_ADD`, `VEC3_SCALE`):
 *   retail builds each result in a compiler temporary.  `change_color_scale_eff` is called with its scale fifth
 *   (`EFT_COLOR_SCALE`; the owner's prologue saves f1 between r6 and r7).  The empty-result setters test the builder's
 *   result with `return`; `eft029_start_effects` picks its parameter through a `switch`; the frame counters are
 *   pre-incremented into a local before the key reads; the charge-act test is `(u16)(act + (u16)-24) <= 6`; the
 *   eft030 alpha is a `u8` local and its fade `x / 8.0f`; the `default:` case of the variant switch comes first.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "fn_8004CAD8.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/effect.h" /* effect_move / change_paramscale_eff (rule 2) */
#include "ef/eft_res.h" /* eft_res_slot_get / eft_res_models_spawn / eft_res_model_get (rule 2) */
#include "ef/eft001.h" /* mtx34_set_trans (rule 2) */
#include "ef/eft002.h" /* eft_effect_foreach_pm_scale (rule 2) */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2) */
#include "ef/change_color_scale_eff.h" /* change_color_scale_eff (rule 2) */
#include "ef/fn_800CDB2C.h" /* ran_suu (rule 2) */
#include "Pl/Pl_master_ck.h" /* Pl_master_ck (rule 2) */
#include "Pl/fn_80273B14.h" /* fn_802748C8 */
#include "Pl/pl_act.h" /* the player act queries (rule 2) */
#include "Pl/fn_802840DC.h"
#include "sound/se_req.h" /* the player SE requests (rule 2) */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */
#include "g3d/g3d_scnroot.h" /* VEC2_ctor (rule 2) */
#include "rotLocalMatX.h" /* rotLocalMatX (rule 2) */
#include "stage/get_now_mapno.h" /* get_now_mapno (rule 2) */
#include "ef/eft029_fx.h"

/* The player's physics block (`_PLW::physics_0x13C`): the joint character sits at +0x04.  size: 0x144 */
struct _EFT029_PL_PHYS {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr;
};

/* The eft029 work block (0x24 bytes from the builder, 0x28 from `eft029_set_kaihou`): the pooled effects and either
 * the scale/colour view of the effect types or the model view of the model types.  size: 0x28 */
struct _EFT029FX_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[2];
    /* +0x0C */ union {
        struct {   /* the model view (types 5, 6, 10, 18); size: 0x1C */
            /* +0x0C */ MHchar* model;              /* the pooled model (`eft_res_model_get`) */
            /* +0x10 */ u8 alpha;                   /* the model's key alpha */
            /* +0x11 */ _GXColor kcolor;            /* the model's TEV constant colour */
            /* +0x15 */ u8 pad_0x15[0x3];
            /* +0x18 */ nw4r::math::VEC3 model_scale;
            /* +0x24 */ f32 rise;                   /* the model's height above the joint */
        };
        struct {   /* the effect view; size: 0x14 */
            /* +0x0C */ u8 stage;                   /* the player's charge stage this frame */
            /* +0x0D */ u8 stage_prev;              /* and last frame */
            /* +0x0E */ u8 pad_0x0E[0x2];
            /* +0x10 */ f32 scale[2];               /* the particle scale pair */
            /* +0x18 */ _GXColor color;
            /* +0x1C */ f32 scale_step;             /* the per-frame shrink of the last stage */
        };
    };
};

extern "C" {
_EFT* eft029_create(_PLW* plw, u8 type, f32 scale);
void eft029_release(_EFT* self);
void eft029_release_effects(_EFT* self);
void eft029_release_model(_EFT* self);
void eft029_dispatch(_EFT* self);
void eft029_start_effects(_EFT* self);
void eft029_start_model(_EFT* self);
void eft029_move(_EFT* self);
void eft029_move_charge(_EFT* self);
void eft029_move_model_keys(_EFT* self);
void eft029_move_model_swing(_EFT* self);
void eft029_move_effects(_EFT* self);
void eft029_move_charge2(_EFT* self);
void eft029_move_hold(_EFT* self);
void eft029_state_next(_EFT* self);
void eft029_free(_EFT* self);
BOOL eft029_spawn_ck(u8 type, _PLW* plw);
}

extern "C" {
/* The unit's `.sdata`: the per-type effect colours and the colour/alpha key strips. */
u8 eft029_color_0[4] = { 0xA7, 0x71, 0x00, 0xFF };
u8 eft029_color_2[4] = { 0xFF, 0x00, 0x00, 0x5A };
u8 eft029_color_3[4] = { 0xFF, 0x28, 0x27, 0xFF };
u8 eft029_color_4[4] = { 0xFF, 0x00, 0x00, 0x5A };
u8 eft029_color_9[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
u8 eft029_color_14[4] = { 0x8C, 0xFF, 0xFF, 0xFF };
u8 eft029_color_15[4] = { 0xFF, 0xFF, 0xD4, 0xC8 };
u8 eft029_color_17[4] = { 0xFF, 0xFF, 0xD4, 0xFF };
u8 eft029_rgb_keys_19[8] = { 0x00, 0xFF, 0xFF, 0x7F, 0x1E, 0xFF, 0xA3, 0x00 };
u8 eft029_color_20[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
u8 eft029_color_21[4] = { 0xEF, 0xFF, 0x5B, 0x65 };
u8 eft029_color_22[8] = { 0xFF, 0x50, 0x50, 0xC8, 0x00, 0x00, 0x00, 0x00 };
u8 eft029_alpha_keys_5[8] = { 0x00, 0x00, 0x03, 0x23, 0x06, 0x00, 0xFF, 0x00 };
u8 eft029_alpha_keys_10[8] = { 0x00, 0x00, 0x03, 0x7F, 0x06, 0x00, 0xFF, 0x00 };
u8 eft029_alpha_keys_18[8] = { 0x00, 0x08, 0x04, 0x30, 0x0C, 0x00, 0xFF, 0x00 };

/* The unit's `.data`: per type, the two effect ids, the release and move kinds, the colour tables, the key strips and
 * the pooled effect count. */
u16 eft029_effect_ids_a[24] = {
    0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00FF, 0x00FF, 0x00F3, 0x00F5, 0x02AE, 0x00FF, 0x00F7,
    0x00F6, 0x02AF, 0x0314, 0x0315, 0x031A, 0x0337, 0x00FF, 0x0314, 0x019B, 0x00F7, 0x00F7, 0x0000,
};
u16 eft029_effect_ids_b[24] = {
    0xFFFF, 0xFFFF, 0x0343, 0x0344, 0x0345, 0x0344, 0xFFFF, 0xFFFF, 0x0343, 0xFFFF, 0xFFFF, 0x0345,
    0x0344, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0345, 0x0345, 0x0000,
};
u8 eft029_release_kinds[24] = {
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
};
u8 eft029_move_kinds[24] = {
    0, 0, 0, 0, 0, 1, 2, 3, 3, 3, 1, 3, 3, 3, 4, 3, 3, 3, 1, 5, 3, 0, 0, 0,
};
u8 eft029_colors_7[12] = { 0xA7, 0x71, 0x00, 0xFF, 0xFF, 0xDB, 0x61, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
u8 eft029_colors_8[12] = { 0xFF, 0xFF, 0xFF, 0x32, 0xFF, 0xDA, 0x2F, 0x50, 0xFF, 0x00, 0x00, 0x5A };
u8 eft029_colors_11[12] = { 0xFF, 0xFF, 0xFF, 0x26, 0xFF, 0xDA, 0x2F, 0x40, 0xFF, 0x00, 0x00, 0x46 };
u8 eft029_colors_12[12] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDA, 0x2F, 0xFF, 0xFF, 0x00, 0x00, 0xFF };
u8* eft029_color_tables[24] = {
    eft029_color_0, NULL, eft029_color_2, eft029_color_3, eft029_color_4, eft029_color_3, NULL, eft029_colors_7,
    eft029_colors_8, eft029_color_9, NULL, eft029_colors_11, eft029_colors_12, NULL, eft029_color_14, eft029_color_15,
    NULL, eft029_color_17, NULL, eft029_rgb_keys_19, eft029_color_20, eft029_color_21, eft029_color_22, NULL,
};
f32 eft029_rise_keys_18[12] = { 0.0f, -80.0f, 2.0f, -76.0f, 4.0f, -70.0f, 6.0f, -63.0f, 12.0f, 45.0f, -1.0f, 45.0f };
f32 eft029_spin_keys_18[6] = { 0.0f, 0.0f, 12.0f, 180.0f, -1.0f, 180.0f };
f32 eft029_scale_keys_5[16] = {
    0.0f, 0.01f, 0.01f, 0.01f, 3.0f, 0.3f, 0.3f, 0.3f, 6.0f, 2.3f, 2.3f, 2.3f, -1.0f, 2.3f, 2.3f, 2.3f,
};
f32 eft029_scale_keys_10[16] = {
    0.0f, 0.02f, 0.02f, 0.02f, 3.0f, 0.6f, 0.3f, 0.6f, 6.0f, 3.5f, 2.5f, 3.5f, -1.0f, 3.5f, 2.5f, 3.5f,
};
f32 eft029_scale_keys_14[6] = { 0.0f, 0.0f, 20.0f, 0.6f, -1.0f, 0.6f };
f32 eft029_scale_keys_18[12] = { 0.0f, 0.8f, 0.8f, 0.8f, 12.0f, 2.7f, 2.7f, 2.7f, -1.0f, 2.7f, 2.7f, 2.7f };
f32 eft029_scale_keys_19[8] = { 0.0f, 0.6f, 12.0f, 1.0f, 30.0f, 0.5f, -1.0f, 0.5f };
f32* eft029_key_tables[23] = {
    NULL, NULL, NULL, NULL, NULL, eft029_scale_keys_5, NULL, NULL, NULL, NULL, eft029_scale_keys_10, NULL,
    NULL, NULL, eft029_scale_keys_14, NULL, NULL, NULL, eft029_scale_keys_18, eft029_scale_keys_19, NULL, NULL, NULL,
};
u8 eft029_effect_counts[24] = {
    1, 1, 2, 2, 2, 2, 1, 1, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 0,
};
}


/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)

/* `change_color_scale_eff` with its parameters in the owner's order: the scale is the fifth (its own prologue saves f1
 * between r6 and r7, and the payload stores it before the position). */
#define EFT_COLOR_SCALE \
    ((void (*)(nw4r::ef::Effect*, u8, _GXColor*, _GXColor*, f32, nw4r::math::VEC3*, u8))change_color_scale_eff)

#pragma peephole off
#pragma optimization_level 4
#pragma pool_data off

/* 0x8011AD58 (0x2C): Builds the record of `type` at half scale. */
extern "C" void eft029_set_half(_PLW* plw, u8 type)
{
    if (eft029_create(plw, type, 0.5f) == NULL) {
        return;
    }
}

/* 0x8011AD84 (0x28): Builds the record of `type` at `scale`. */
void eft029_set_scale(_PLW* plw, u8 type, f32 scale)
{
    if (eft029_create(plw, type, scale) == NULL) {
        return;
    }
}

/* 0x8011ADAC (0x210): Gates on the type and the player's area, allocates the record and its work block, sets the
 * scale (types 7 and 8 read the player's charge level) and installs the release and per-frame hooks. */
extern "C" _EFT* eft029_create(_PLW* plw, u8 type, f32 scale)
{
    _EFT* rec;
    _EFT029FX_WORK* work;

    if (eft029_spawn_ck(type, plw) == 0) {
        return NULL;
    }
    if (plw->area_0x16 != get_now_areano()) {
        return NULL;
    }
    rec = (_EFT*)eft_res_slot_get(0x24);
    if (rec == NULL) {
        return NULL;
    }
    rec->area_0x44 = plw->area_0x16;
    work = (_EFT029FX_WORK*)rec->work_0x38;
    work->scale[1] = scale;
    work->scale[0] = scale;
    switch (type) {
    case 2:
        if (rec->area_0x44 == get_now_areano()) {
            pl_se_req_charge_level(plw, 2);
        }
        break;
    case 7:
        switch ((u8)pl_stagger_level_get(plw)) {
        case 1:
            rec->demo_flag_0x08 = 0;
            break;
        case 2:
            rec->demo_flag_0x08 = 1;
            break;
        case 3:
            rec->demo_flag_0x08 = 2;
            break;
        default:
            return NULL;
        }
        break;
    case 8:
        switch ((u8)pl_stagger_level_get(plw)) {
        case 0:
        case 1:
            work->scale[0] = work->scale[1] = 0.7f * scale;
            break;
        case 2:
            work->scale[1] = scale;
            work->scale[0] = scale;
            break;
        case 3:
            work->scale[0] = work->scale[1] = 1.3f * scale;
            break;
        }
        break;
    }
    work->count = eft029_effect_counts[type];
    rec->field_0x03 = 29;
    rec->type_0x02 = type;
    rec->source_0x30 = plw;
    rec->timer_0x0C = 0;
    rec->field_0x10 = 0;
    eft_state_flags_set(rec, 1, 0);
    rec->release_0x40 = eft029_release;
    rec->dispatch_0x34 = eft029_dispatch;
    return rec;
}

/* 0x8011AFBC (0x110): Builds the release-burst model record of `type` (type 6 only from charge level 3). */
void eft029_set_kaihou(_PLW* plw, u8 type)
{
    _EFT* rec;
    _EFT029FX_WORK* work;

    if (eft029_spawn_ck(type, plw) == 0) {
        return;
    }
    if (plw->area_0x16 != get_now_areano()) {
        return;
    }
    if (type == 6 && (u8)pl_charge_stage_get(plw) < 3) {
        return;
    }
    rec = (_EFT*)eft_res_slot_get(0x28);
    if (rec == NULL) {
        return;
    }
    rec->type_0x02 = type;
    rec->release_0x40 = eft029_release;
    rec->dispatch_0x34 = eft029_dispatch;
    work = (_EFT029FX_WORK*)rec->work_0x38;
    work->count = 2;
    work->model = (MHchar*)eft_res_model_get();
    if (work->model == NULL) {
        eft_res_slot_release(rec);
        return;
    }
    rec->field_0x03 = 29;
    rec->timer_0x0C = 0;
    rec->field_0x10 = 0;
    rec->source_0x30 = plw;
    rec->area_0x44 = plw->area_0x16;
    eft_state_flags_set(rec, 0, 0);
}

/* 0x8011B0CC (0x30): Releases the record by its type's release kind. */
extern "C" void eft029_release(_EFT* self)
{
    switch (eft029_release_kinds[self->type_0x02]) {
    case 0:
        eft029_release_effects(self);
        break;
    case 1:
        eft029_release_model(self);
        break;
    }
}

/* 0x8011B0FC (0x3C): Returns the pooled effects. */
extern "C" void eft029_release_effects(_EFT* self)
{
    _EFT029FX_WORK* work = (_EFT029FX_WORK*)self->work_0x38;

    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* 0x8011B138 (0x48): Returns the pooled effects and the model. */
extern "C" void eft029_release_model(_EFT* self)
{
    _EFT029FX_WORK* work = (_EFT029FX_WORK*)self->work_0x38;

    push_eft_effect_heap_num(work->effects, work->count);
    eft_res_models_release(&work->model, 1);
    work->count = 0;
}

static void eft029_start(_EFT* self);

/* Runs the record's state: start, move, end, release. */
extern "C" void eft029_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft029_start(self);
        break;
    case 1:
        eft029_move(self);
        break;
    case 2:
        eft029_state_next(self);
        break;
    case 3:
        eft029_free(self);
        break;
    }
}

/* Starts the record by its type's release kind (effects or model). */
static void eft029_start(_EFT* self)
{
    switch (eft029_release_kinds[self->type_0x02]) {
    case 0:
        eft029_start_effects(self);
        break;
    case 1:
        eft029_start_model(self);
        break;
    }
}

/* 0x8011B1EC (0x2FC): Creates the effect type's effects, takes the player's facing and colour, places them on the
 * player's joint and plays the type's start sound. */
extern "C" void eft029_start_effects(_EFT* self)
{
    _EFT029FX_WORK* work = (_EFT029FX_WORK*)self->work_0x38;
    _PLW* plw = (_PLW*)self->source_0x30;
    nw4r::math::VEC3 unused;
    u8* col;
    s32 i;

    VEC3_ctor(&unused);
    self->state_0x05++;
    {
        u16 param;
        switch (self->type_0x02) {
        case 20:
            param = 21;
            break;
        default:
            param = 30;
            break;
        }
        work->effects[0] = res_eft_create(eft029_effect_ids_a[self->type_0x02], param, 0);
    }
    if (work->effects[0] == NULL) {
        eft029_free(self);
        return;
    }
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }
    self->flag_0x01 = 1;
    eft_rot_vec_copy(&self->rot_0x24, &plw->rot_0x54);
    switch (self->type_0x02) {
    case 0:
        self->timer_0x0C = (s16)pl_act_frame_get(plw, 0, 14);
    case 14:
        col = eft029_color_tables[self->type_0x02];
        work->color.r = col[0];
        work->color.g = col[1];
        work->color.b = col[2];
        work->color.a = col[3];
        eft029_set_scale(plw, 1, 0.5f);
        break;
    case 2:
    case 3:
        work->effects[1] = res_eft_create(eft029_effect_ids_b[self->type_0x02], 30, 0);
        if (work->effects[1] == NULL) {
            eft029_free(self);
            return;
        }
        self->timer_0x0C = 0;
        break;
    case 4:
    case 8:
    case 11:
    case 12:
    case 21:
    case 22:
        work->effects[1] = res_eft_create(eft029_effect_ids_b[self->type_0x02], 30, 0);
        if (work->effects[1] == NULL) {
            eft029_free(self);
            return;
        }
        break;
    case 7:
    case 9:
    case 15:
    case 17:
    case 20:
        col = eft029_color_tables[self->type_0x02];
        work->color.r = col[self->demo_flag_0x08 * 4];
        work->color.g = col[self->demo_flag_0x08 * 4 + 1];
        work->color.b = col[self->demo_flag_0x08 * 4 + 2];
        work->color.a = col[self->demo_flag_0x08 * 4 + 3];
        self->timer_0x0C = 10;
        break;
    }
    ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
    }
    if (self->area_0x44 == get_now_areano()) {
        switch (self->type_0x02) {
        case 0:
        case 14:
            pl_se_req_charge_start(plw, 0);
            break;
        case 10:
        case 13:
            pl_se_req_charge_7(plw);
            break;
        case 19:
            pl_se_req_charge_2(plw);
            break;
        case 22:
            pl_se_req_charge_burst(plw, &self->pos_0x18);
            break;
        }
    }
    eft029_move(self);
}

/* 0x8011B4E8 (0x2B8): Creates the model type's effects and model, seeds its scale keys, places it on the player's
 * joint and clears its constant alpha. */
extern "C" void eft029_start_model(_EFT* self)
{
    _EFT029FX_WORK* work = (_EFT029FX_WORK*)self->work_0x38;
    nw4r::math::VEC3 unused;
    s32 i;
    _PLW* plw = (_PLW*)self->source_0x30;
    f32* keys;

    VEC3_ctor(&unused);
    self->state_0x05++;
    ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
    switch (self->type_0x02) {
    case 5:
        work->effects[0] = res_eft_create(0xF6, 30, 0);
        if (work->effects[0] == NULL) {
            eft029_free(self);
            return;
        }
        work->effects[1] = res_eft_create(eft029_effect_ids_b[self->type_0x02], 30, 0);
        if (work->effects[1] == NULL) {
            eft029_free(self);
            return;
        }
        keys = eft029_key_tables[self->type_0x02];
        self->timer_0x0C = 9;
        self->rot_0x24.y = ran_suu(0);
        work->model_scale.x = keys[0];
        work->model_scale.y = keys[1];
        work->model_scale.z = keys[2];
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        }
        break;
    case 6:
        self->timer_0x0C = 2;
        self->rot_0x24.x = 0xE71D;
        self->rot_0x24.y = plw->field_0x058;
        work->model_scale.x = 0.6f;
        work->model_scale.y = 1.1f;
        work->model_scale.z = 0.9f;
        work->rise = 90.0f;
        work->alpha = 0;
        break;
    case 10:
        keys = eft029_key_tables[self->type_0x02];
        SetRootMtxTrans(work->effects[0], &self->pos_0x18);
        self->timer_0x0C = 6;
        self->rot_0x24.y = ran_suu(0);
        work->model_scale.x = keys[0];
        work->model_scale.y = keys[1];
        work->model_scale.z = keys[2];
        break;
    case 18:
        keys = eft029_key_tables[self->type_0x02];
        self->timer_0x0C = 12;
        self->rot_0x24.y = plw->field_0x058;
        work->model_scale.x = keys[1];
        work->model_scale.y = keys[2];
        work->model_scale.z = keys[3];
        break;
    }
    if (res_eft_model_create(work->model, 0x23, 0x18) == NULL) {
        eft029_free(self);
        return;
    }
    copyVec3(&work->model->pos_0x04, &self->pos_0x18);
    work->model->getTevKColor(0, (_GXTevKColorID)3, &work->kcolor);
    work->kcolor.a = 0;
    work->model->setTevKColor(0, (_GXTevKColorID)3, &work->kcolor);
    self->flag_0x01 = 1;
    eft029_move(self);
}

/* 0x8011B7A0 (0x60): Runs the type's move handler. */
extern "C" void eft029_move(_EFT* self)
{
    switch (eft029_move_kinds[self->type_0x02]) {
    case 0:
        eft029_move_charge(self);
        break;
    case 1:
        eft029_move_model_keys(self);
        break;
    case 2:
        eft029_move_model_swing(self);
        break;
    case 3:
        eft029_move_effects(self);
        break;
    case 4:
        eft029_move_charge2(self);
        break;
    case 5:
        eft029_move_hold(self);
        break;
    }
}

/* 0x8011B800 (0x4E0): Follows the player's charge: type 0 steps the charge stages (colour, scale and sound per stage)
 * while the player holds a charge act, then places and moves the effects and tints the colour types. */
extern "C" void eft029_move_charge(_EFT* self)
{
    u8 done = 0;
    nw4r::math::VEC2 uv;
    nw4r::math::VEC3 ofs;
    _PLW* plw;
    _EFT029FX_WORK* work;
    _GXColor col;
    u8* src;
    s32 i;
    u16 act;

    VEC2_ctor(&uv);
    VEC3_ctor(&ofs);
    plw = (_PLW*)self->source_0x30;
    work = (_EFT029FX_WORK*)self->work_0x38;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if ((s32)self->type_0x02 == 0) {
        work->stage = pl_charge_stage_get(plw);
        if (plw->field_0x00A == 4 &&
            ((u16)((act = plw->act_no) + (u16)-24) <= 6 || (u16)(act + (u16)-44) <= 3 || act == 35 || act == 64)) {
            switch (self->field_0x06) {
            case 0:
                if (work->stage == 1 && (s32)work->stage_prev == 0) {
                    work->color.r = 225;
                    work->color.g = 219;
                    work->color.b = 97;
                    eft029_set_scale(plw, 1, 1.0f);
                    if (self->area_0x44 == get_now_areano()) {
                        pl_se_req_charge_start(plw, 1);
                        pl_se_req_charge_level(plw, 0);
                    }
                    self->field_0x06++;
                }
                break;
            case 1:
                if (work->stage == 2) {
                    if (work->stage_prev == 1) {
                        work->color.r = 255;
                        work->color.g = 255;
                        work->color.b = 255;
                        eft029_set_scale(plw, 1, 1.0f);
                        if (self->area_0x44 == get_now_areano()) {
                            pl_se_req_charge_level(plw, 1);
                        }
                        self->field_0x06++;
                    }
                } else {
                    work->scale[0] = 1.0f;
                    work->scale[1] = 1.0f;
                }
                break;
            case 2:
                if (work->stage == 3) {
                    if (work->stage_prev == 2) {
                        self->field_0x06++;
                    }
                } else {
                    work->scale[0] = 1.5f;
                    work->scale[1] = 1.5f;
                }
                break;
            case 3:
                if (work->stage == 2) {
                    if (work->stage_prev == 3) {
                        self->timer_0x0C = plw->field_0x28 + 1;
                        self->field_0x06++;
                        work->scale_step = 1.0f / self->timer_0x0C;
                    }
                } else {
                    work->scale[0] = 1.5f;
                    work->scale[1] = 1.5f;
                }
            case 4:
                work->scale[0] -= work->scale_step;
                work->scale[1] -= work->scale_step;
                break;
            }
            if (plw->charge_gauge_0x310 == self->timer_0x0C) {
                eft029_set_kaihou(plw, 5);
            }
            EFT_COLOR_SCALE(work->effects[0], 1, &work->color, &work->color, 5000.0f, &self->pos_0x18, 1);
        } else {
            self->flag_0x01 = 0;
            self->state_0x05++;
            pl_se_req_charge_end(plw);
            return;
        }
        work->stage_prev = work->stage;
    }
    if ((s32)self->type_0x02 == 22) {
        ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
        setVector3(&ofs, 0.0f, -20.0f, 0.0f);
        rotVecX(&ofs, plw->param_0x54);
        addVec3To(&self->pos_0x18, &ofs);
    } else {
        ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
    }
    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        if (effect_move(work->effects[i]) == 0) {
            done++;
        }
        eft_effect_foreach_pm_scale(work->effects[i], (u32)work->scale, 1);
    }
    if (done >= work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    switch (self->type_0x02) {
    case 2:
    case 3:
    case 4:
        src = eft029_color_tables[self->type_0x02];
        col.r = src[0];
        col.g = src[1];
        col.b = src[2];
        col.a = src[3];
        change_color_eff(work->effects[0], &self->pos_0x18, col);
        break;
    case 21:
    case 22:
        src = eft029_color_tables[self->type_0x02];
        col.r = src[0];
        col.g = src[1];
        col.b = src[2];
        col.a = src[3];
        for (i = 0; i < work->count; i++) {
            change_color_eff(work->effects[i], &self->pos_0x18, col);
        }
        break;
    }
    eft_res_models_spawn(self, (void**)work->effects, 1, work->count, 0);
}

/* 0x8011BCE0 (0x370): Runs the model down its timer: the scale and alpha keys per type, the spin (types 5 and 10) or
 * the rising swing (type 18), then pushes the model's colour and transform and moves the effects. */
extern "C" void eft029_move_model_keys(_EFT* self)
{
    _PLW* plw = (_PLW*)self->source_0x30;
    _EFT029FX_WORK* work = (_EFT029FX_WORK*)self->work_0x38;
    nw4r::math::VEC3 ofs;
    _GXColor col;
    u8* src;
    s32 i;
    f32 ang;
    s32 frame;

    VEC3_ctor(&ofs);
    if (--self->timer_0x0C < 0 || eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    frame = ++self->field_0x10;
    getKeyData3(eft029_key_tables[self->type_0x02], frame, &work->model_scale.x, &work->model_scale.y,
                &work->model_scale.z);
    switch (self->type_0x02) {
    case 5:
        work->alpha = eftGetKeyAlpha(eft029_alpha_keys_5, self->field_0x10);
        self->rot_0x24.y += 0x2000;
        if (self->timer_0x0C < 2) {
            self->field_0x06 = 1;
        }
        ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
        break;
    case 10:
        work->alpha = eftGetKeyAlpha(eft029_alpha_keys_10, self->field_0x10);
        self->rot_0x24.y += 0x2000;
        ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
        break;
    case 18:
        work->alpha = eftGetKeyAlpha(eft029_alpha_keys_18, self->field_0x10);
        ang = 65536.0f * getKeyData(eft029_spin_keys_18, self->field_0x10) / 360.0f;
        self->rot_0x24.y = (u16)(0.5f + ang);
        copyVec3(&self->pos_0x18, &plw->vec_0x03C);
        setVector3(&ofs, 0.0f, 0.0f, 5.0f);
        rotVecY(&ofs, plw->field_0x058);
        addVec3To(&self->pos_0x18, &ofs);
        self->pos_0x18.y += getKeyData(eft029_rise_keys_18, self->field_0x10);
        break;
    }
    if (self->field_0x06 < 1) {
        work->kcolor.a = work->alpha;
        work->model->setTevKColor(0, (_GXTevKColorID)3, &work->kcolor);
        copyVec3(&work->model->pos_0x04, &self->pos_0x18);
        work->model->field_0x2C = self->rot_0x24.y;
        copyVec3(&work->model->scale_0x1C, &work->model_scale);
        work->model->move(0);
        eft_res_models_spawn(self, (void**)&work->model, 2, 1, 0);
    }
    if ((s32)self->type_0x02 != 18) {
        src = eft029_color_tables[self->type_0x02];
        if (src != NULL) {
            col.r = src[0];
            col.g = src[1];
            col.b = src[2];
            col.a = src[3];
            change_color_eff(work->effects[0], &self->pos_0x18, col);
        }
        for (i = 0; i < work->count; i++) {
            if (work->effects[i] != NULL) {
                SetRootMtxTrans(work->effects[i], &self->pos_0x18);
                if (effect_move(work->effects[i]) == 1) {
                    eft_res_models_spawn(self, (void**)&work->effects[i], 1, 1, 0);
                }
            }
        }
    }
}

/* 0x8011C050 (0x29C): Swings the model up and out (stage 0 brightens it, stage 1 fades it), then pushes its colour,
 * scale and transform above the player's joint 7. */
extern "C" void eft029_move_model_swing(_EFT* self)
{
    nw4r::math::VEC3 ofs;
    nw4r::math::MTX34 mtx;
    _PLW* plw;
    _EFT029FX_WORK* work;

    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    plw = (_PLW*)self->source_0x30;
    work = (_EFT029FX_WORK*)self->work_0x38;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->timer_0x0C--;
    switch (self->field_0x06) {
    case 0:
        work->alpha += 15;
        work->model_scale.x -= 0.065f;
        work->model_scale.y += 0.3f;
        work->model_scale.z += 0.3f;
        work->rise -= 26.5f;
        if (self->timer_0x0C < 0) {
            work->alpha = 30;
            work->model_scale.y = 1.7f;
            work->model_scale.z = 1.5f;
            self->timer_0x0C = 4;
            self->field_0x06++;
        }
        break;
    case 1:
        work->alpha -= 7;
        work->model_scale.x -= 0.065f;
        work->model_scale.y += 0.575f;
        work->model_scale.z += 1.1f;
        work->rise -= 26.5f;
        if (self->timer_0x0C < 0) {
            work->alpha = 0;
            work->model_scale.x = 0.2f;
            work->model_scale.y = 4.0f;
            work->model_scale.z = 6.0f;
            work->rise = -70.0f;
            self->field_0x06++;
        }
        break;
    case 2:
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    mtx34_identity(&mtx);
    rotMatrixY(self->rot_0x24.y, &mtx);
    setVector3(&ofs, 0.0f, work->rise, 100.0f);
    mulVecMat(&ofs, &mtx);
    rotLocalMatX(self->rot_0x24.x, &mtx);
    ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(7, &self->pos_0x18);
    self->pos_0x18.y = plw->ground_y_0x060;
    addVec3To(&self->pos_0x18, &ofs);
    mtx34_set_trans(&mtx, &self->pos_0x18);
    work->kcolor.a = work->alpha;
    work->model->setTevKColor(0, (_GXTevKColorID)3, &work->kcolor);
    copyVec3(&work->model->scale_0x1C, &work->model_scale);
    work->model->move2(&mtx, 0);
    eft_res_models_spawn(self, (void**)&work->model, 2, 1, 0);
}

/* 0x8011C2EC (0x308): Places the effects (type 16 on the player's position with a rising spin, type 20 on joint 11,
 * the rest on joint 2), moves them at the record's scale and tints them (types 8 and 11-13 by the charge level). */
extern "C" void eft029_move_effects(_EFT* self)
{
    _GXColor col;
    s32 i;
    u8 done = 0;
    s8 level;
    _PLW* plw;
    _EFT029FX_WORK* work;
    nw4r::math::MTX34 mtx;
    u8* src;

    MTX34_ctor(&mtx);
    plw = (_PLW*)self->source_0x30;
    work = (_EFT029FX_WORK*)self->work_0x38;
    switch (self->type_0x02) {
    case 16:
        copyVec3(&self->pos_0x18, &plw->vec_0x03C);
        self->pos_0x18.y += 50.0f;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effects[0]->SetRootMtx(mtx);
        break;
    case 20:
        ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(11, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        }
        break;
    default:
        ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        }
        break;
    }
    switch (self->type_0x02) {
    case 7:
        if (--self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        eft_effect_foreach_pm_scale(work->effects[0], (u32)work->scale, 1);
        effect_move(work->effects[0]);
        change_color_eff(work->effects[0], &self->pos_0x18, work->color);
        break;
    case 8:
    case 11:
    case 12:
    case 13:
    case 16:
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff(work->effects[i], work->scale[0]);
            if (effect_move(work->effects[i]) == 0) {
                done++;
            }
        }
        if (done >= work->count) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        switch (self->type_0x02) {
        case 8:
        case 11:
        case 12:
            level = pl_stagger_level_get(plw) - 1;
            if (level < 0) {
                level = 0;
            }
            src = eft029_color_tables[self->type_0x02];
            col.r = src[level * 4];
            col.g = src[level * 4 + 1];
            col.b = src[level * 4 + 2];
            col.a = src[level * 4 + 3];
            change_color_eff(work->effects[0], &self->pos_0x18, col);
            break;
        }
        break;
    default:
        eft_effect_foreach_pm_scale(work->effects[0], (u32)work->scale, 1);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        change_color_eff(work->effects[0], &self->pos_0x18, work->color);
        break;
    }
    eft_res_models_spawn(self, (void**)work->effects, 1, work->count, 0);
}

/* 0x8011C5F4 (0x398): Follows the second weapon class's charge while the player holds a charge act: the stages set
 * the scale, sound and release burst; the effect sits on the stage's joint and moves at the record's scale. */
extern "C" void eft029_move_charge2(_EFT* self)
{
    nw4r::math::VEC2 uv;
    nw4r::math::VEC3 tip;
    _PLW* plw;
    _EFT029FX_WORK* work;
    u32 joint;
    f32 s;
    s32 frame;

    VEC2_ctor(&uv);
    VEC3_ctor(&tip);
    plw = (_PLW*)self->source_0x30;
    work = (_EFT029FX_WORK*)self->work_0x38;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    work->stage = pl_charge_stage2_get(plw);
    work->color.r = 230;
    work->color.g = 255;
    work->color.b = 150;
    if (plw->field_0x00A == 4) {
        switch (plw->act_no) {
        case 7:
        case 8:
        case 9:
        case 10:
        case 17:
        case 29:
        case 30:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 55:
        case 56:
            switch (self->field_0x06) {
            case 0:
                joint = 2;
                if (work->stage == 1) {
                    if ((s32)work->stage_prev == 0) {
                        eft029_set_scale(plw, 1, 1.0f);
                        if (self->area_0x44 == get_now_areano()) {
                            pl_se_req_charge_start(plw, 1);
                            pl_se_req_charge_level(plw, 0);
                        }
                        self->field_0x06++;
                    }
                } else {
                    frame = ++self->field_0x10;
                    s = getKeyData(eft029_key_tables[self->type_0x02], frame);
                    work->scale[1] = s;
                    work->scale[0] = s;
                }
                break;
            case 1:
                joint = 2;
                if (work->stage == 2) {
                    if (work->stage_prev == 1) {
                        eft029_set_scale(plw, 13, 1.5f);
                        eft029_set_scale(plw, 15, 1.0f);
                        if (self->area_0x44 == get_now_areano()) {
                            pl_se_req_charge_level(plw, 1);
                        }
                        self->field_0x06++;
                        self->field_0x07 = 0;
                        self->timer_0x0C = 4;
                        pl_impact_pos_get(plw, &tip);
                        if (self->area_0x44 == get_now_areano()) {
                            pl_se_req_charge_tip(plw, &tip);
                        }
                    }
                } else {
                    work->scale[0] = 1.0f;
                    work->scale[1] = 1.0f;
                }
                break;
            default:
                if ((s32)self->field_0x07 == 0 && --self->timer_0x0C < 0) {
                    self->field_0x07++;
                    eft029_set_scale(plw, 17, 2.0f);
                }
                joint = 11;
                work->scale[0] = 0.5f;
                work->scale[1] = 0.5f;
                break;
            }
            EFT_COLOR_SCALE(work->effects[0], 1, &work->color, &work->color, 5000.0f, &self->pos_0x18, 1);
            work->stage_prev = work->stage;
            ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(joint, &self->pos_0x18);
            SetRootMtxTrans(work->effects[0], &self->pos_0x18);
            if (effect_move(work->effects[0]) == 0) {
                self->flag_0x01 = 0;
                self->state_0x05++;
                return;
            }
            eft_effect_foreach_pm_scale(work->effects[0], (u32)work->scale, 1);
            eft_res_models_spawn(self, (void**)work->effects, 1, work->count, 0);
            return;
        }
    }
    pl_se_req_charge_end(plw);
    self->flag_0x01 = 0;
    self->state_0x05++;
}

/* 0x8011C98C (0x1E8): While the player holds act 34 or 44 the effects grow along the type's scale keys on joint 2 and
 * take the type's colour keys; otherwise the record ends with its stop sound. */
extern "C" void eft029_move_hold(_EFT* self)
{
    u8 done = 0;
    nw4r::math::VEC2 uv;
    _PLW* plw;
    _EFT029FX_WORK* work;
    _GXColor col;
    s32 i;
    f32 s;

    VEC2_ctor(&uv);
    plw = (_PLW*)self->source_0x30;
    work = (_EFT029FX_WORK*)self->work_0x38;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (plw->field_0x00A == 4 && (plw->act_no == 34 || plw->act_no == 44)) {
        s = getKeyData(eft029_key_tables[self->type_0x02], self->field_0x10);
        work->scale[1] = s;
        work->scale[0] = s;
    } else {
        self->flag_0x01 = 0;
        self->state_0x05++;
        pl_se_req_charge_end_b(plw);
        return;
    }
    self->field_0x10++;
    ((_EFT029_PL_PHYS*)plw->physics_0x13C)->chr.get_joint_wpos(2, &self->pos_0x18);
    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        if (effect_move(work->effects[i]) == 0) {
            done++;
        }
        eft_effect_foreach_pm_scale(work->effects[i], (u32)work->scale, 1);
    }
    if (done >= work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    eftGetKeyRGB(eft029_color_tables[self->type_0x02], self->field_0x10, &col.r, &col.g, &col.b);
    col.a = 0xFF;
    change_color_eff(work->effects[0], &self->pos_0x18, col);
    eft_res_models_spawn(self, (void**)work->effects, 1, work->count, 0);
}

/* 0x8011CB74 (0x10): Advances the record's state. */
extern "C" void eft029_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011CB84 (0x4): Releases the record. */
extern "C" void eft029_free(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x8011CB88 (0x9C): Whether the player may spawn `type`: the master player for the common types, the charge level
 * above 1 (type 9) or above 2 (type 16). */
extern "C" BOOL eft029_spawn_ck(u8 type, _PLW* plw)
{
    switch (type) {
    case 0:
    case 1:
    case 3:
    case 14:
    case 15:
    case 17:
        return Pl_master_ck(plw) != 0;
    case 9:
        return (u32)(u8)pl_stagger_level_get(plw) > 1U;
    case 16:
        return (u32)(u8)pl_stagger_level_get(plw) > 2U;
    default:
        return TRUE;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * the eft030 stage model (0x8011CC24..0x8011D448)
 * ------------------------------------------------------------------------------------------------- */

/* The eft030 work block (`eft_res_slot_get(0x14)`): one pooled model, its scale and the stage's path segments the model
 * appears on.  size: 0x14 */
struct _EFT030_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[1];
    /* +0x08 */ f32 scale;
    /* +0x0C */ Vec* path;      /* `seg_count` segments, each a start and an end point */
    /* +0x10 */ u8 seg_count;
    /* +0x11 */ u8 pad_0x11[0x3];
};

extern "C" {
void eft030_release(_EFT* self);
void eft030_dispatch(_EFT* self);
void eft030_start(_EFT* self);
void eft030_move(_EFT* self);
void eft030_state_next(_EFT* self);
void eft030_free(_EFT* self);
}

extern "C" {
/* The eft030 `.data`: the stage paths per area (map 5), each segment a start and an end point, and their segment
 * counts. */
Vec eft030_path_0[4] = {
    { -22000.0f, 19250.0f, -37000.0f }, { -30000.0f, 19250.0f, -37000.0f },
    { 38000.0f, 26000.0f, -37000.0f }, { 60000.0f, 26000.0f, -37000.0f },
};
Vec eft030_path_1[2] = { { -1700.0f, 44000.0f, -65000.0f }, { 83000.0f, 44000.0f, -43000.0f } };
Vec eft030_path_3[2] = { { 820.0f, 34600.0f, -47415.0f }, { 58691.0f, 34600.0f, -24746.0f } };
Vec eft030_path_6[2] = { { -2800.0f, 170000.0f, -170000.0f }, { 130000.0f, 170000.0f, -100000.0f } };
Vec* eft030_paths[12] = {
    eft030_path_0, eft030_path_1, NULL, eft030_path_3, NULL, NULL, eft030_path_6, NULL, NULL, NULL, NULL, NULL,
};
u8 eft030_path_counts[16] = { 2, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

/* 0x8011CC24 (0xC4): Allocates the eft030 record with one pooled model and installs its hooks. */
extern "C" void eft030_set(void)
{
    _EFT* rec;
    _EFT030_WORK* work;
    s32 i;

    rec = (_EFT*)eft_res_slot_get(0x14);
    if (rec == NULL) {
        return;
    }
    rec->dispatch_0x34 = eft030_dispatch;
    rec->release_0x40 = eft030_release;
    work = (_EFT030_WORK*)rec->work_0x38;
    work->count = 1;
    for (i = 0; i < work->count; i++) {
        work->models[0] = (MHchar*)eft_res_model_get();
        if (work->models[0] == NULL) {
            eft_res_slot_release(rec);
            return;
        }
    }
    rec->source_0x30 = NULL;
    rec->field_0x03 = 30;
    eft_state_flags_set(rec, 8, 0);
}

/* 0x8011CCE8 (0x54): Returns the pooled models. */
extern "C" void eft030_release(_EFT* self)
{
    s32 i;
    _EFT030_WORK* work = (_EFT030_WORK*)self->work_0x38;

    for (i = 0; i < work->count; i++) {
        eft_res_models_release(work->models, 1);
    }
}

/* 0x8011CD3C (0x3C): Runs the record's state: start, move, end, release. */
extern "C" void eft030_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft030_start(self);
        break;
    case 1:
        eft030_move(self);
        break;
    case 2:
        eft030_state_next(self);
        break;
    case 3:
        eft030_free(self);
        break;
    }
}

/* 0x8011CD78 (0x14C): Creates the model, takes the area's path (map 5 only) and resets the model's scale and rotation;
 * no path ends the record. */
extern "C" void eft030_start(_EFT* self)
{
    _EFT030_WORK* work = (_EFT030_WORK*)self->work_0x38;
    s32 i;

    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->models[0], 38, 76) == NULL) {
            eft030_free(self);
            return;
        }
    }
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->area_0x44 = get_now_areano();
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    if (get_now_mapno() == 5) {
        work->path = eft030_paths[self->area_0x44];
        work->seg_count = eft030_path_counts[self->area_0x44];
    }
    if (work->path == NULL) {
        eft030_state_next(self);
        return;
    }
    for (i = 0; i < work->count; i++) {
        setVector3(&work->models[0]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        work->models[0]->field_0x28 = 0;
        work->models[0]->field_0x2C = 0;
        work->models[0]->field_0x28 = 0;
        work->models[0]->ready = 1;
    }
    eft030_move(self);
}

#pragma fp_contract off
/* 0x8011CEC4 (0x570): Waits, then shows the model at a random point of a random path segment with a random variant,
 * height, spin and scale, and fades it in while it rises. */
extern "C" void eft030_move(_EFT* self)
{
    nw4r::math::VEC3 a;
    nw4r::math::VEC3 b;
    s32 j;
    s32 seg;
    f32 t;
    f32 rise;
    u8 alpha;
    s32 i;
    _EFT030_WORK* work = (_EFT030_WORK*)self->work_0x38;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    _GXColor col = { 0xFF, 0xFF, 0xFF, 0xFF };
    for (i = 0; i < work->count; i++) {
        switch (self->field_0x06) {
        case 0:
            if (--self->timer_0x0C < 0) {
                self->field_0x06++;
                self->field_0x07 = ran_suu(1) % 5;
                self->demo_flag_0x08 = 0;
                self->timer_0x0C = 12;
                switch (self->field_0x07) {
                default:
                    rise = 0.0f;
                    self->field_0x10 = (ran_suu(1) & 0x7FF) - 0x400;
                    work->scale = 1.0f;
                    break;
                case 2:
                    rise = 7300.0f + (ran_suu(1) & 0x3FF);
                    self->field_0x10 = (ran_suu(1) & 0xFFF) - 0x800;
                    work->scale = 1.0f;
                    break;
                case 3:
                    rise = 6000.0f + (ran_suu(1) & 0x3FF) * 3;
                    self->field_0x10 = (ran_suu(1) & 0xFFF) - 0x800;
                    work->scale = 1.0f;
                    break;
                case 4:
                    rise = 7000.0f + (ran_suu(1) & 0x7FF);
                    self->field_0x10 = 0;
                    work->scale = 0.7f + 0.0003f * (ran_suu(1) & 0x3FF);
                    break;
                }
                t = ran_suu(1) / 65535.0f;
                seg = (u8)(ran_suu(1) % work->seg_count) * 2;
                vec_to_mh_vec3(&a, &work->path[seg]);
                vec_to_mh_vec3(&b, &work->path[seg] + 1);
                copyVec3(&self->pos_0x18, &VEC3_ADD(&VEC3_SCALE(&VEC3_SUB(&b, &a), t), &a));
                self->pos_0x18.y += rise;
                setVector3(&work->models[0]->scale_0x1C, work->scale, work->scale, work->scale);
                work->models[0]->field_0x30 = self->field_0x10;
                for (j = 0; j < 10; j++) {
                    if (j == self->field_0x07 * 2 + 1) {
                        work->models[0]->setVisibility(j + 1, 1);
                    } else {
                        work->models[0]->setVisibility(j + 1, 0);
                    }
                }
            }
            break;
        case 1:
            if (self->timer_0x0C < 8) {
                alpha = 255.0f * (self->timer_0x0C / 8.0f);
            } else {
                alpha = 255;
            }
            col.a = alpha;
            work->models[0]->setMatColor(0, (_GXChannelID)4, col, 0);
            if (--self->timer_0x0C <= 0) {
                self->field_0x06 = 0;
                self->timer_0x0C = (ran_suu(1) & 0x3F) + 1;
            } else if (self->timer_0x0C < 11 && self->demo_flag_0x08 == 0) {
                work->models[0]->setVisibility(self->field_0x07 * 2 + 2, 0);
                work->models[0]->setVisibility(self->field_0x07 * 2 + 1, 1);
                self->demo_flag_0x08++;
            }
            copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
            work->models[0]->move(0);
            eft_res_models_spawn(self, (void**)work->models, 2, 1, 0);
            break;
        }
    }
}

#pragma fp_contract reset

/* 0x8011D434 (0x10): Advances the record's state. */
extern "C" void eft030_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011D444 (0x4): Releases the record. */
extern "C" void eft030_free(_EFT* self)
{
    eft_res_slot_release(self);
}
