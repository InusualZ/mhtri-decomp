/* ef/fn_80114E34.cpp - the eft020 effect family: the creator `fn_80114E34` (a 0x20-byte `_EFT_WORK_A` block from
 *   `eft_res_slot_get`, `+0x03 = 20`, release `fn_80114F34` at +0x40, dispatch `fn_80114F70` at +0x34) and its
 *   per-frame bodies.
 * RANGE. .text 0x80114E34-0x801153D0 (8 functions); extab 0x8000C3D4-0x8000C3F4, extabindex 0x800264A8-0x800264D8,
 *   .ctors 0x8056F310-0x8056F314, .data 0x805A00B8-0x805A0208, .bss 0x806A4538-0x806A4548, .sdata2
 *   0x80796A28-0x80796A30.  Families 21-22 from 0x801153D0 are `ef/eft022_fx.cpp`, 23-24 `ef/eft023_fx.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps the `li r0` + `psq_l` epilogue and the
 *   raw `clrlwi` chains; playbook 39).
 * NAMES. The map has only `fn_` stems here, so the definitions are `extern "C"` in a C++ unit.
 *   GUESS (the data, map `lbl_805A00B8`..`lbl_805A01E0`, `lbl_806A4538`): `eft020_effect_ids`,
 *   `eft020_effect_counts`, `eft020_rgb_keys_aN`/`_bN`, `eft020_rgb_key_tables_a`/`_b`, `eft020_origin`,
 *   `EftOriginVec3`.  The map's `fn_801153B8` is MWCC's `__sinit_\fn_80114E34_cpp` (rename filed; it follows the file
 *   name).
 * RESIDUALS. none in bytes: every row and section matches (`.bss` 0xC of the claimed 0x10, the tail the link's 8-byte
 *   fill).  The `__sinit` row reads 0 in the report until the map names it.
 * SHAPES. `fn_80115100` caches `source_0x30` in a typed local per case (`plw`, `enemy`; case 2 reloads it), tests
 *   `(s32)self->type_0x02 != 9` (`cmpwi`), passes `work->color[i]` by value with no local copy, and declares
 *   `i, dead, flags, work, plw, enemy` in that order.
 *   The origin vector is a file-scope `EftOriginVec3` whose constructor calls `setVec3`: MWCC emits `__sinit` (a tail
 *   call to `setVec3`), its `.ctors` word and the 0.0f/-5.0f pool.
 *   `fn_80114E34` takes `u8` parameters with explicit `(u8)` casts, and its float guard is `scale <= 0.0f`
 *   (`fcmpo` + `cror`), not `==` (`fcmpu`).
 *   `fn_80114FAC` declares `MTX34 mtx; s32 i; u16 id; s32 n; _EFT_WORK_A* work;` in that order.
 *   `fn_80115100` selects the key with a ternary on the loaded value (`(i == 0) ? eft020_rgb_key_tables_a[type] :
 *   eft020_rgb_key_tables_b[type]`); selecting the table pointer first merges the two load arms.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "enemy/fn_8012BDF4.h"
#include "sys_mem.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/fn_802693C4.h"
#include "ef/fn_8011722C.h"
#include "unsplit/g3d.h"
#include "unsplit/sound.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_80114E34_types.h"

struct _EFT_CHARA_SRC;

/* The eft020 work block. size: 0x20 */
struct _EFT_WORK_A {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[3];
    /* +0x10 */ _GXColor color[2];
    /* +0x18 */ u32 joint;
    /* +0x1C */ f32 scale;
};

/* size: 0x144 (lower bound) */

/* The mode-2 effect source: its `MHchar` sits at +8. */
struct _EFT_CHARA_SRC {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ MHchar chr_0x08;
};

/* size: 0xB18 (lower bound) */

/* ---------------------------------------------------------------------------------------------------
 * externs - the callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

/* The unit's own forward declarations (plain names, `extern "C"`). */
extern "C" _EFT* fn_80114E34(u8, u8, f32);
extern "C" void fn_80114F34(_EFT*);
extern "C" void fn_80114F70(_EFT*);
extern "C" void fn_80114FAC(_EFT*);
extern "C" void fn_80115100(_EFT*);
extern "C" void fn_801153A4(_EFT*);
extern "C" void fn_801153B4(_EFT*);

/* The library entry points, declared by their map spelling (a mangled callee is not evidence of a
 * C++ unit; only a mangled definition is). */
extern "C" _EFT* eft_res_slot_get(u32 pool_id);
extern "C" void eft_res_slot_release(void* self);
extern "C" void eft_state_flags_set(_EFT* self, u8 a, u8 b);

extern "C" u32 eft_res_spawn_gate_ck(_EFT* self, u32 mode);
extern "C" u32 eft_stage_spawn_flag(_PLW* plw);
extern "C" void eft_res_models_spawn(void* self, void* list, u32 mode, s32 count, u32 arg);

extern "C" u8 GameMode_ck(void);

extern "C" void addVec3To(nw4r::math::VEC3* a, nw4r::math::VEC3* b);

/* fn_8011722C comes from its owner's header (rule 2). */

/* The `MHchar` members keep the map spelling: their owner (`pl.h`'s struct) carries no methods (rule 9); the other
 * library entry points come from their owners' headers. */
extern "C" void get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3(void* chr, u32 joint,
                                                             nw4r::math::VEC3* out);

/* The unit's `.data`: the effect id and effect count per type, the two RGB key runs (frame, r, g, b) the
 * per-frame tint walks, and the per-type tables of those runs (one per effect). */
extern "C" {
u16 eft020_effect_ids[10] = {
    0x01A4, 0x01F6, 0x01F7, 0x01A4, 0x01A4, 0x01A4, 0x01A4, 0x01A4, 0x01A4, 0x039A,
};
u8 eft020_effect_counts[10] = {
    3, 1, 1, 3, 3, 3, 3, 3, 3, 1,
};
u8 eft020_rgb_keys_a0[16] = {
    0, 0xDA, 0xFF, 0xD0,
    9, 0xA2, 0xE0, 0xA9,
    19, 0x64, 0xD6, 0x64,
    255, 0x64, 0xD6, 0x64,
};
u8 eft020_rgb_keys_a1[16] = {
    0, 0x3F, 0xBF, 0xFF,
    9, 0x3F, 0xBF, 0xFF,
    19, 0x3F, 0x9F, 0xFF,
    255, 0x3F, 0x9F, 0xFF,
};
u8 eft020_rgb_keys_a2[16] = {
    0, 0xBB, 0xBC, 0xFF,
    9, 0xBB, 0xBC, 0xFF,
    19, 0x70, 0x23, 0xFF,
    255, 0x70, 0x23, 0xFF,
};
u8 eft020_rgb_keys_a3[16] = {
    0, 0xFF, 0xAA, 0x50,
    9, 0xFF, 0xAA, 0x50,
    19, 0xFF, 0xEB, 0x96,
    255, 0xFF, 0xEB, 0x96,
};
u8 eft020_rgb_keys_a4[16] = {
    0, 0xDF, 0xFF, 0xFF,
    9, 0xDF, 0xDF, 0xFF,
    19, 0x7F, 0xFF, 0xFF,
    255, 0x7F, 0xFF, 0xFF,
};
u8 eft020_rgb_keys_a5[16] = {
    0, 0xFF, 0xDE, 0x5F,
    9, 0xFF, 0x25, 0x10,
    19, 0xFF, 0xAF, 0x5F,
    255, 0xFF, 0xAF, 0x5F,
};
u8 eft020_rgb_keys_a6[16] = {
    0, 0xFF, 0x16, 0x16,
    9, 0xFF, 0x67, 0x4F,
    19, 0xFF, 0xAF, 0x74,
    255, 0xFF, 0xAF, 0x74,
};
u8 eft020_rgb_keys_b0[16] = {
    0, 0x00, 0xB4, 0xB4,
    15, 0xF0, 0xFF, 0xD6,
    46, 0x5F, 0xFF, 0xBF,
    255, 0x5F, 0xFF, 0xBF,
};
u8 eft020_rgb_keys_b1[16] = {
    0, 0x9F, 0xBF, 0xFF,
    15, 0xBF, 0xDF, 0xFF,
    46, 0x5F, 0x9F, 0xFF,
    255, 0x5F, 0x9F, 0xFF,
};
u8 eft020_rgb_keys_b2[16] = {
    0, 0xFF, 0xFF, 0xFF,
    15, 0xEF, 0xDF, 0xFF,
    46, 0x9C, 0x7F, 0xFF,
    255, 0x9C, 0x7F, 0xFF,
};
u8 eft020_rgb_keys_b3[16] = {
    0, 0xFF, 0xEB, 0x64,
    15, 0xFF, 0xEB, 0x64,
    46, 0xFF, 0xC8, 0x82,
    255, 0xFF, 0xC8, 0x82,
};
u8 eft020_rgb_keys_b4[16] = {
    0, 0xDF, 0xFF, 0xFF,
    15, 0x7F, 0xFF, 0xFF,
    46, 0x7F, 0xFF, 0xFF,
    255, 0x7F, 0xFF, 0xFF,
};
u8 eft020_rgb_keys_b5[16] = {
    0, 0xFF, 0xC8, 0x7C,
    15, 0xFF, 0x8F, 0x1F,
    46, 0xFF, 0x30, 0x00,
    255, 0xFF, 0x30, 0x00,
};
u8 eft020_rgb_keys_b6[16] = {
    0, 0xFF, 0xC4, 0xA8,
    15, 0xFF, 0x82, 0x5B,
    46, 0xFF, 0x5F, 0x00,
    255, 0xFF, 0x5F, 0x00,
};
u8* eft020_rgb_key_tables_a[10] = {
    eft020_rgb_keys_a0, NULL, NULL, eft020_rgb_keys_a1,
    eft020_rgb_keys_a2, eft020_rgb_keys_a3, eft020_rgb_keys_a4, eft020_rgb_keys_a5,
    eft020_rgb_keys_a6, NULL,
};
u8* eft020_rgb_key_tables_b[10] = {
    eft020_rgb_keys_b0, NULL, NULL, eft020_rgb_keys_b1,
    eft020_rgb_keys_b2, eft020_rgb_keys_b3, eft020_rgb_keys_b4, eft020_rgb_keys_b5,
    eft020_rgb_keys_b6, NULL,
};
}

/* A vector built through `setVec3` (0x80041E8C), so a file-scope one is constructed by `__sinit`.  size: 0xC */
struct EftOriginVec3 : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    EftOriginVec3(f32 x, f32 y, f32 z)
    {
        setVec3(this, x, y, z);
    }
};

/* The effect origin, 5 units below the player's joint; MWCC's `__sinit` builds it. */
static EftOriginVec3 eft020_origin(0.0f, -5.0f, 0.0f);

#pragma peephole off

/* Creates the area's eft020 record, seeds its work block and installs the two hooks. */
extern "C" _EFT* fn_80114E34(u8 type, u8 area, f32 scale)
{
    _EFT* effect;
    _EFT_WORK_A* work;

    if ((u8)area != (u8)get_now_areano()) {
        return 0;
    }
    if (scale <= 0.0f) {
        return 0;
    }
    effect = eft_res_slot_get(0x20);
    if (effect == 0) {
        return 0;
    }
    work = (_EFT_WORK_A*)effect->work_0x38;
    work->count = eft020_effect_counts[(u8)type];
    work->scale = scale;
    effect->field_0x03 = 20;
    effect->timer_0x0C = 0;
    effect->type_0x02 = type;
    effect->area_0x44 = area;
    eft_state_flags_set(effect, 0, 0);
    effect->release_0x40 = fn_80114F34;
    effect->dispatch_0x34 = fn_80114F70;
    return effect;
}

/* Hands the record's pooled effects back and clears its count. */
extern "C" void fn_80114F34(_EFT* self)
{
    _EFT_WORK_A* work = (_EFT_WORK_A*)self->work_0x38;

    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* Runs the state handler the record's `state_0x05` selects. */
extern "C" void fn_80114F70(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_80114FAC(self);
    case 1:
        return fn_80115100(self);
    case 2:
        return fn_801153A4(self);
    case 3:
        return fn_801153B4(self);
    }
}

/* Spawns the record's effects and places the record's position. */
extern "C" void fn_80114FAC(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    s32 i;
    u16 id;
    s32 n;
    _EFT_WORK_A* work;

    MTX34_ctor(&mtx);
    work = (_EFT_WORK_A*)self->work_0x38;
    self->state_0x05++;
    if (self->type_0x02 == 0) {
        if ((u8)GameMode_ck() == 2) {
            id = 0x9FB;
            n = 3;
        } else {
            id = eft020_effect_ids[self->type_0x02];
            n = 9;
        }
    } else {
        id = eft020_effect_ids[self->type_0x02];
        n = 9;
    }
    for (i = 0; i < work->count; i++) {
        work->effects[i] = res_eft_create(id, n, 0);
        if (work->effects[i] == 0) {
            fn_801153B4(self);
            return;
        }
        id++;
    }
    work->color[0].a = 0xFF;
    work->color[1].a = 0xFF;
    self->flag_0x01 = 1;
    fn_80115100(self);
    switch (self->field_0x07) {
    case 0:
        if ((u8)GameMode_ck() == 2) {
            fn_800DA93C(&self->pos_0x18);
            return;
        }
        fn_800DA8F4(&self->pos_0x18);
        return;
    case 1:
        fn_800DA95C(&self->pos_0x18);
        return;
    }
}

/* Moves the record's effects each frame and tints them from the shared key tables. */
extern "C" void fn_80115100(_EFT* self)
{
    nw4r::math::VEC3 v;
    nw4r::math::MTX34 mtx;
    s32 i;
    s32 dead;
    s32 flags;
    _EFT_WORK_A* work;
    _PLW* plw;
    _ENEMY_WORK* enemy;

    dead = 0;
    work = (_EFT_WORK_A*)self->work_0x38;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);
    flags = 1;
    switch (self->mode_0x08) {
    case 0:
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        plw = (_PLW*)self->source_0x30;
        copyVec3(&v, &eft020_origin);
        get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3(&plw->physics_0x13C->chr_0x04, 3, &self->pos_0x18);
        addVec3To(&self->pos_0x18, &v);
        if (Pl_master_ck(plw) == 1) {
            flags = eft_stage_spawn_flag(plw) | 1;
        }
        break;
    case 1:
        enemy = (_ENEMY_WORK*)self->source_0x30;
        if (em_work_die_ck(enemy) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        get_joint_wpos_em(enemy, work->joint, &self->pos_0x18);
        break;
    case 2:
        if (eft_res_spawn_gate_ck(self, 2) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3(
            &((_EFT_CHARA_SRC*)self->source_0x30)->chr_0x08, work->joint, &self->pos_0x18);
        break;
    default:
        break;
    }
    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        change_paramscale_eff(work->effects[i], work->scale);
        if (effect_move(work->effects[i]) == 0) {
            dead++;
        }
    }
    if (dead == work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if ((u32)(self->type_0x02 - 1) > 1U && (s32)self->type_0x02 != 9) {
        self->timer_0x0C++;
        for (i = 0; i < 2; i++) {
            eftGetKeyRGB((i == 0) ? eft020_rgb_key_tables_a[self->type_0x02]
                                  : eft020_rgb_key_tables_b[self->type_0x02],
                                         self->timer_0x0C, &work->color[i].r, &work->color[i].g,
                                         &work->color[i].b);
            change_color_eff(work->effects[i], &self->pos_0x18, work->color[i]);
        }
    }
    eft_res_models_spawn(self, &work->effects[0], flags, work->count, 0);
}

/* Bumps the record's `state_0x05` state index. */
extern "C" void fn_801153A4(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the record: the family-20 state-3 step, the release `eft_res_slot_release` performs. */
extern "C" void fn_801153B4(_EFT* self)
{
    eft_res_slot_release(self);
}
