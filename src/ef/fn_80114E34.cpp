/* ef/fn_80114E34.cpp - the eft020 effect family: the creator `fn_80114E34` (a 0x20-byte `_EFT_WORK_A` block from
 *   `eft_res_slot_get`, `+0x03 = 20`, release `fn_80114F34` at +0x40, dispatch `fn_80114F70` at +0x34) and its
 *   per-frame bodies.
 * RANGE. .text 0x80114E34-0x801153D0 (8 functions); extab 0x8000C3D4-0x8000C3F4, extabindex 0x800264A8-0x800264D8,
 *   .ctors 0x8056F310-0x8056F314, .data 0x805A00B8-0x805A0208, .bss 0x806A4538-0x806A4548, .sdata2
 *   0x80796A28-0x80796A30.  Families 21-24 from 0x801153D0 are `ef/eft022_fx.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps the `li r0` + `psq_l` epilogue and the
 *   raw `clrlwi` chains; playbook 39).
 * NAMES. The map has only `fn_` stems here, so the definitions are `extern "C"` in a C++ unit.
 * RESIDUALS. 1 partial row: `fn_80115100` is 28 bytes long, the `_GXColor` by-value temporary growing the frame.
 *   flipcheck: `.bss`/`.ctors`/`.data`/`.sdata2` claimed, not emitted; `.text` 0x5B8 against the claimed 0x59C and
 *   differing; extabindex differs in 1 byte.
 * SHAPES. `fn_80114E34` takes `u8` parameters with explicit `(u8)` casts, and its float guard is `scale <= 0.0f`
 *   (`fcmpo` + `cror`), not `==` (`fcmpu`).
 *   `fn_80114FAC` declares `MTX34 mtx; s32 i; u16 id; s32 n; _EFT_WORK_A* work;` in that order.
 *   `fn_80115100` selects the key with a ternary on the loaded value (`(i == 0) ? lbl_805A01B8[type] :
 *   lbl_805A01E0[type]`); selecting the table pointer first merges the two load arms.
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
extern "C" void fn_801153B8(void);

/* The library entry points, declared by their map spelling (a mangled callee is not evidence of a
 * C++ unit; only a mangled definition is). */
extern "C" _EFT* eft_res_slot_get(u32 pool_id);
extern "C" void eft_res_slot_release(void* self);
extern "C" void eft_state_flags_set(_EFT* self, u8 a, u8 b);

extern "C" u32 eft_res_spawn_gate_ck(_EFT* self, u32 mode);
extern "C" u32 fn_800F9380(_PLW* plw);
extern "C" void eft_res_models_spawn(void* self, void* list, u32 mode, s32 count, u32 arg);

extern "C" u8 GameMode_ck(void);

extern "C" void addVec3To(nw4r::math::VEC3* a, nw4r::math::VEC3* b);

/* fn_8011722C comes from its owner's header (rule 2). */

/* The `MHchar` members keep the map spelling: their owner (`pl.h`'s struct) carries no methods (rule 9); the other
 * library entry points come from their owners' headers. */
extern "C" void get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3(void* chr, u32 joint,
                                                             nw4r::math::VEC3* out);

/* The shared pool.  The `.data` tables are unsized arrays (large-data `lis`/`addi` addressing in the
 * target); the `.sdata`/`.sdata2` scalars stay scalars so their `@sda21` loads are unchanged. */
extern "C" u16 lbl_805A00B8[];
extern "C" u8 lbl_805A00CC[];
extern "C" u8* lbl_805A01B8[];
extern "C" u8* lbl_805A01E0[];

extern "C" nw4r::math::VEC3 lbl_806A4538;

extern "C" f32 lbl_80796A28;
extern "C" f32 lbl_80796A2C;

#pragma peephole off

/* Creates the area's eft020 record, seeds its work block and installs the two hooks. */
extern "C" _EFT* fn_80114E34(u8 type, u8 area, f32 scale)
{
    _EFT* effect;
    _EFT_WORK_A* work;

    if ((u8)area != (u8)get_now_areano()) {
        return 0;
    }
    if (scale <= lbl_80796A28) {
        return 0;
    }
    effect = eft_res_slot_get(0x20);
    if (effect == 0) {
        return 0;
    }
    work = (_EFT_WORK_A*)effect->work_0x38;
    work->count = lbl_805A00CC[(u8)type];
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
            id = lbl_805A00B8[self->type_0x02];
            n = 9;
        }
    } else {
        id = lbl_805A00B8[self->type_0x02];
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
    s32 dead;
    _EFT_WORK_A* work;
    s32 i;
    s32 flags;
    _GXColor color;

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
        copyVec3(&v, &lbl_806A4538);
        get_joint_wpos__6MHcharFUlPQ34nw4r4math4VEC3(
            &((_PLW*)self->source_0x30)->physics_0x13C->chr_0x04, 3, &self->pos_0x18);
        addVec3To(&self->pos_0x18, &v);
        if (Pl_master_ck((_PLW*)self->source_0x30) == 1) {
            flags = fn_800F9380((_PLW*)self->source_0x30) | 1;
        }
        break;
    case 1:
        if (em_work_die_ck((_ENEMY_WORK*)self->source_0x30) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        get_joint_wpos_em((_ENEMY_WORK*)self->source_0x30,
                                                                work->joint, &self->pos_0x18);
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
    if ((u32)(self->type_0x02 - 1) > 1U && self->type_0x02 != 9) {
        self->timer_0x0C++;
        for (i = 0; i < 2; i++) {
            eftGetKeyRGB((i == 0) ? lbl_805A01B8[self->type_0x02]
                                                  : lbl_805A01E0[self->type_0x02],
                                         self->timer_0x0C, &work->color[i].r, &work->color[i].g,
                                         &work->color[i].b);
            color = work->color[i];
            change_color_eff(work->effects[i],
                                                                             &self->pos_0x18, color);
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

/* Zeroes the shared effect-origin vector. */
extern "C" void fn_801153B8(void)
{
    setVec3(&lbl_806A4538, lbl_80796A28, lbl_80796A2C, lbl_80796A28);
}
