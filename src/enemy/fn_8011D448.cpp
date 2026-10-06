/* enemy/fn_8011D448.cpp - the enemy effect-spawner band: the pooled `_EFT` creators an `_ENEMY_WORK` owns, with
 *   their release/dispatch callbacks (the `ef/eft001.cpp` family shape), then the enemy part/gauge helpers and the
 *   action/state machine.
 * RANGE. .text 0x8011D448-0x801251D0 (101 functions); .data 0x805A0C10-0x805A0FB0 (the u16 effect-id tables and four
 *   jump tables), .sdata 0x807919C8-0x807919D0, .sdata2 0x80796C08-0x80796C50, extab, extabindex.
 * SEAM. Unproven at both edges: `ef/eft029_fx.cpp` below (.text 0x8011AD58-0x8011D448), whose `.data`/`.sdata`/
 *   `.sdata2` runs end where this unit's begin (0x805A0C10, 0x807919C8, 0x80796C08), with no `.ctors` word on either
 *   side; `enemy/em_common.cpp` above; `tudiscover` reports only weak signals across the range.
 * NAMES. `em_parts_damage_level_get` (0x8011E9DC) is a runtime-dump name; the dump's `JASSeqCtrl::setIntrMask` at
 *   0x8011F230 is a library-signature match on a 16-byte body, not evidence.  `em_parts_damage_add`, the other
 *   `em_parts_*` and the `em_event_settle*` names are GUESSES from their bodies.
 *   GUESS (from each body and its callers): em_status_bits_set
 * RESIDUALS. 75 rows unwritten: 0x8011DA24-0x8011DE2C, 0x8011DE5C-0x8011E378, 0x8011E3A8-0x8011E51C,
 *   0x8011E7F4-0x8011E9DC, 0x8011E9F0-0x8011F230, 0x8011F240-0x801251D0.
 *  - `fn_8011D690`, `fn_8011D7B0`: ours stores the scale and the id word at +0x8C/+0x90 where retail stores +0xC/
 *    +0x10; `fn_8011D690` also stores one word more and `fn_8011D7B0` moves r6/r7 earlier;
 *  - `fn_8011D8F0`: retail hoists `&work[i].slots` out of the outer loop and advances both pointers by 0x44, ours
 *    re-forms `+8` in the body (132 B against 140 B);
 *  - `fn_8011D9B8`: retail tests `state_0x05 == 0` and lays its body after the four stage tail calls, ours lets the
 *    state-0 body fall through first (92 B against 108 B);
 *  - `fn_8011E658`: retail loads `amount_0x7A0` a second time for the `+=`.
 *   flipcheck: `.data`/`.sdata` claimed, not emitted; `.sdata2`/`.text`/extab/extabindex short of the claim; ours
 *   emits a 4-byte `.sbss` the unit does not claim.
 * SHAPES. `#pragma peephole off` over the bodies (with the pass on `fn_8011E530` fuses `clrlwi` + `slwi` into
 *   `clrlslwi` and `fn_8011D558` loses the `clrlwi` before its stores); the creators take `u32` type/timer arguments
 *   (retail masks at the store) and `fn_8011D558` ends `(f32 scale, u8 timer)` (its prologue's `fmr f31,f1` comes
 *   before `mr r29,r7`); `event_demo_ck`/`get_em_scale` are declared at C++ scope for their mangled relocations; the
 *   creators' common skeleton for the unwritten rows is in docs/enemy.md.
 */

#include "types.h"

#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"

#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft_res.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* Retail keeps the unfused `clrlwi` + `slwi` and `clrlwi` + `stb`/`stw` pairs the peephole pass folds
 * (`fn_8011E530`'s part index, `fn_8011D558`'s type/timer narrowing; playbook 39). */
#pragma peephole off

/* This unit's own `.data` (0x805A0C10-0x805A0FB0) and `.sdata` (0x807919C8-0x807919D0), declared and never defined
 * (playbook 29). */
extern "C" {
u16 lbl_805A0C10[]; /* type -> effect id block, 0x8011DE5C's default arm */
u16 lbl_805A0C20[];
u16 lbl_805A0C30[];
u16 lbl_805A0C40[];
u16 lbl_805A0C50[];
u8 lbl_805A0C60[];  /* type -> work state word (the 0x20-pool creators) */
f32 lbl_805A0C88[]; /* type -> target vector component */
u8 lbl_805A0CB8[];  /* type -> family tag (0 = pool, 1 = flat run) */
u8 lbl_805A0CC8[];
u8 lbl_807919C8[];  /* family tag -> re-arm delay */
f32 lbl_80796C08;   /* 0.0f */
}

/* This unit's own members the dispatchers tail-call, and `em_get_mot_no` (`enemy/em_common.cpp`) and
 * `get_camera_pos` (`camera/camera_main.cpp`). */
extern "C" {
void fn_8011DA24(_EFT* self);
void fn_8011DB74(_EFT* self);
void fn_8011DE2C(_EFT* self);
void fn_8011DE5C(_EFT* self);
void fn_8011E138(_EFT* self);
void fn_8011E378(_EFT* self);
void fn_8011E3A8(_EFT* self);
void fn_8011E51C(_EFT* self);
void fn_8011E52C(_EFT* self);
void fn_8011E658(_ENEMY_WORK* self, s32 step, s32 limit);
void fn_8011D8F0(_EFT* self);
void fn_8011D97C(_EFT* self);
void fn_8011D8C0(_EFT* self);
void fn_8011D9B8(_EFT* self);
_EFT* fn_8011D558(_ENEMY_WORK* self, u32 kind, u32 id0, u32 id1, f32 scale, u8 timer);
s32 em_get_mot_no(_ENEMY_WORK* enemy);
void get_camera_pos(nw4r::math::VEC3* out);


/* The three 3-float helpers come from `mh3_pad.h` (`mh3_pad.cpp`). */
}

/* `get_em_scale` (0x80135940, `enemy/em_common.cpp`), declared at C++ scope so it mangles to the map's
 * `get_em_scale__FP11_ENEMY_WORK`. */
f32 get_em_scale(_ENEMY_WORK* enemy);

/* `event_demo_ck` (0x803C4814, `lobby/lb_server_sel_trans.cpp`), declared at C++ scope for `event_demo_ck__Fv`;
 * retail compares its result unsigned (`cmplwi r3,0x1`), so it returns `u32`. */
u32 event_demo_ck();

/* The effect record's per-family work blocks. */

/* One 0x44-byte group of the pooled family: entry 0's `entry_count` is the number of groups, every
 * entry's `slot_count` the number of live slots that follow it. size: 0x44 */
struct _EFT_FX_GROUP {
    /* +0x00 */ s32 entry_count;
    /* +0x04 */ s32 slot_count;
    /* +0x08 */ nw4r::ef::Effect* slots[15];
};

/* The flat family: one count, then that many slots. size: 0x04 + 4n */
struct _EFT_FX_POOL {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[1];
};

/* The `0xA8` pool's work block.  `state_0x00` is the family tag the creator writes, `ids_0x90` the
 * two id words the same creator passes and `fn_8011E530` indexes by the spawner's part number.
 * size: 0xA4 (lower bound: +0x98 is the highest offset the written functions read) */
struct _EFT_FX_FULL {
    /* +0x00 */ u32 state_0x00;
    /* +0x04 */ u8 pad_0x04[0x88];
    /* +0x8C */ f32 scale_0x8C;
    /* +0x90 */ u32 ids_0x90[2];
    /* +0x98 */ nw4r::math::VEC3 target_0x98;
};

/* Bodies. */

/* 0x8011D448 - spawns one pooled effect through fn_8011D558 and points its target vector along the
 * type table's z.  The z component comes from `lbl_805A0C88[type]`; x and y stay at 0.0f. */
extern "C" void fn_8011D448(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, f32 scale)
{
    _EFT* effect = fn_8011D558(self, type, id, id, scale, timer);
    _EFT_FX_FULL* work;

    if (effect != NULL) {
        work = (_EFT_FX_FULL*)effect->work_0x38;
        setVector3(&work->target_0x98, lbl_80796C08, lbl_80796C08, lbl_805A0C88[type]);
    }
}

/* 0x8011D4AC - the same spawn, with the target vector copied from the caller's record instead of the
 * table. */
extern "C" void fn_8011D4AC(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, const nw4r::math::VEC3* pos,
                            f32 scale)
{
    _EFT* effect = fn_8011D558(self, type, id, id, scale, timer);
    _EFT_FX_FULL* work;

    if (effect != NULL) {
        work = (_EFT_FX_FULL*)effect->work_0x38;
        copyVec3(&work->target_0x98, pos);
    }
}

/* 0x8011D4FC - the same spawn, target vector from the type table only.  Distinct from 0x8011D448
 * because the id pair is taken from the caller's two words. */
extern "C" void fn_8011D4FC(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, u32 timer, f32 scale)
{
    _EFT* effect = fn_8011D558(self, type, id0, id1, scale, timer);
    _EFT_FX_FULL* work;

    if (effect != NULL) {
        work = (_EFT_FX_FULL*)effect->work_0x38;
        setVector3(&work->target_0x98, lbl_80796C08, lbl_80796C08, lbl_805A0C88[type]);
    }
}

/* 0x8011D558 - the `0xA8`-pool creator: within the enemy's own area, fills the work block and stamps the record
 * (tag 0x1f, type, timer, owner, area, rotation), installs the release/dispatch pair, marks demo ownership. */
extern "C" _EFT* fn_8011D558(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, f32 scale, u8 timer)
{
    _EFT* effect;
    _EFT_FX_FULL* work;

    if (get_now_areano() != self->act_id) {
        return NULL;
    }

    effect = (_EFT*)eft_res_slot_get(0xA8);
    if (effect == NULL) {
        return NULL;
    }

    work = (_EFT_FX_FULL*)effect->work_0x38;
    work->state_0x00 = 2;
    work->scale_0x8C = scale * get_em_scale(self);
    work->ids_0x90[0] = id0;
    work->ids_0x90[1] = id1;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->timer_0x0C = timer;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    effect->release_0x40 = fn_8011D8C0;
    effect->dispatch_0x34 = fn_8011D9B8;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    return effect;
}

/* 0x8011D690 - the `0x20`-pool creator for the same family: the enemy's own anchor, its pose
 * rotation and the table's work state word. */
extern "C" void fn_8011D690(_ENEMY_WORK* self, u32 type, u32 id, f32 scale)
{
    _EFT* effect;
    _EFT_FX_FULL* work;

    if (get_now_areano() != self->act_id) {
        return;
    }

    effect = (_EFT*)eft_res_slot_get(0x20);
    if (effect == NULL) {
        return;
    }

    work = (_EFT_FX_FULL*)effect->work_0x38;
    work->state_0x00 = lbl_805A0C60[type];
    work->scale_0x8C = scale * get_em_scale(self);
    work->ids_0x90[0] = id;
    work->ids_0x90[1] = 0;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->timer_0x0C = 0;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = fn_8011D8C0;
    effect->dispatch_0x34 = fn_8011D9B8;
}

/* 0x8011D7B0 - the `0x20`-pool creator that belongs to nobody: the caller passes the type, position, rotation
 * and area, so `source_0x30` stays NULL. */
extern "C" void fn_8011D7B0(u32 type, const nw4r::math::VEC3* pos, const _CP_VECTOR* rot, u8 area,
                            u8 demo, f32 scale)
{
    _EFT* effect;
    _EFT_FX_FULL* work;

    if (get_now_areano() != area) {
        return;
    }

    effect = (_EFT*)eft_res_slot_get(0x20);
    if (effect == NULL) {
        return;
    }

    work = (_EFT_FX_FULL*)effect->work_0x38;
    work->state_0x00 = lbl_805A0C60[type];
    work->scale_0x8C = scale;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->source_0x30 = NULL;
    effect->area_0x44 = area;
    effect->rot_0x24 = *rot;
    copyVec3(&effect->pos_0x18, pos);
    effect->timer_0x0C = 0;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = demo;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = fn_8011D8C0;
    effect->dispatch_0x34 = fn_8011D9B8;
}

/* 0x8011D8C0 - the pooled family's `.release` callback: hand the type's slot run back. */
extern "C" void fn_8011D8C0(_EFT* self)
{
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011D8F0(self);
        break;
    case 1:
        fn_8011D97C(self);
        break;
    }
}

/* 0x8011D8F0 - the grouped release: every slot of every 0x44-stride group, one call each, then the
 * group count is cleared. */
extern "C" void fn_8011D8F0(_EFT* self)
{
    _EFT_FX_GROUP* work = (_EFT_FX_GROUP*)self->work_0x38;
    s32 i;
    s32 j;

    for (i = 0; i < work[0].entry_count; i++) {
        for (j = 0; j < work[i].slot_count; j++) {
            push_eft_effect_heap_num(&work[i].slots[j], 1);
        }
    }
    work[0].entry_count = 0;
}

/* 0x8011D97C - the flat release: the whole run in one call, then the count is cleared. */
extern "C" void fn_8011D97C(_EFT* self)
{
    _EFT_FX_POOL* work = (_EFT_FX_POOL*)self->work_0x38;

    push_eft_effect_heap_num(work->slots, work->count);
    work->count = 0;
}

/* 0x8011D9B8 - the family's `.dispatch` callback: `state_0x05` selects the stage, and stage 0 posts on to the
 * type table's first/second handler. */
extern "C" void fn_8011D9B8(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        fn_8011DE2C(self);
        return;
    case 2:
        fn_8011E378(self);
        return;
    case 3:
        fn_8011E52C(self);
        return;
    }
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011DA24(self);
        break;
    case 1:
        fn_8011DB74(self);
        break;
    }
}

/* 0x8011DE2C - the second family's release dispatcher. */
extern "C" void fn_8011DE2C(_EFT* self)
{
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011DE5C(self);
        break;
    case 1:
        fn_8011E138(self);
        break;
    }
}

/* 0x8011E378 - the third family's per-frame dispatcher. */
extern "C" void fn_8011E378(_EFT* self)
{
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011E3A8(self);
        break;
    case 1:
        fn_8011E51C(self);
        break;
    }
}

/* 0x8011E51C - the terminal stage of the shared state machine. */
extern "C" void fn_8011E51C(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011E52C - the terminal release: hand the whole pooled record back. */
extern "C" void fn_8011E52C(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x8011E530 - places `effect` at the enemy's `part` joint: the record's rotation carries the work's target
 * vector, the joint's world position is added, and the result goes into the pooled effect. */
extern "C" void fn_8011E530(_EFT* self, nw4r::ef::Effect* effect, u8 part)
{
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 target;
    nw4r::math::MTX34 mtx;
    _ENEMY_WORK* owner;
    _EFT_FX_FULL* work;

    VEC3_ctor(&joint);
    VEC3_ctor(&target);
    MTX34_ctor(&mtx);

    owner = (_ENEMY_WORK*)self->source_0x30;
    work = (_EFT_FX_FULL*)self->work_0x38;

    cpSetRotMatrix(&self->rot_0x24, &mtx);
    copyVec3(&target, &work->target_0x98);
    mulVecMat(&target, &mtx);
    get_joint_wpos_em(owner, work->ids_0x90[part], &joint);
    addVec3To(&joint, &target);
    fn_800FBB90(&mtx, &joint);
    effect->SetRootMtx(mtx);
}

/* 0x8011E5EC - seeds the enemy's status bits from its static data record. */
extern "C" void fn_8011E5EC(_ENEMY_WORK* self)
{
    self->bits_0x824 = get_enemy_data(self)->bits_0x30;
}

/* 0x8011E620 - sets status bits. */
extern "C" void em_status_bits_set(_ENEMY_WORK* self, u32 mask)
{
    self->bits_0x824 |= mask;
}

/* 0x8011E630 - clears status bits. */
extern "C" void fn_8011E630(_ENEMY_WORK* self, u32 mask)
{
    self->bits_0x824 &= ~mask;
}

/* 0x8011E640 - are all of `mask`'s bits set?  (retail emits the `cntlzw` of
 * `(bits & mask) - mask`, which is this comparison and nothing else.) */
extern "C" u32 fn_8011E640(_ENEMY_WORK* self, u32 mask)
{
    return (self->bits_0x824 & mask) == mask;
}

/* 0x8011E658 - steps the gauge and clamps it to [0, ceiling], never below the data record's per-model ratio
 * (`field_0x7a4 * field_0x7B0`). */
extern "C" void fn_8011E658(_ENEMY_WORK* self, s32 step, s32 ceiling)
{
    s32 value;
    s32 low;

    if (self->amount_0x7A0 > ceiling) {
        ceiling = self->amount_0x7A0;
    }
    value = self->amount_0x7A0 + step;
    self->amount_0x7A0 = value;
    if (value < 0) {
        self->amount_0x7A0 = 0;
    } else if (value > ceiling) {
        self->amount_0x7A0 = ceiling;
    }
    low = (s32)((f32)(s32)self->field_0x7a4 * self->field_0x7B0);
    if (self->amount_0x7A0 < low) {
        self->amount_0x7A0 = low;
    }
}

/* 0x8011E6E4 - steps the gauge by `step` with the data record's ratio as the ceiling. */
extern "C" void fn_8011E6E4(_ENEMY_WORK* self, s32 step)
{
    fn_8011E658(self, step, self->field_0x7a4);
}

/* 0x8011E6EC - steps the gauge on every `period`-th system tick (an unsigned `divwu` modulo: a `u32` counter
 * against the sign-extended `s16`). */
extern "C" void fn_8011E6EC(_ENEMY_WORK* self, s32 step, s16 period, f32 scale)
{
    if (system_w.field_0x0c % period == 0) {
        fn_8011E658(self, step, (s32)((f32)(s32)self->field_0x7a4 * scale));
    }
}

/* 0x8011E760 - forces the gauge to `value`, clamped the same way `fn_8011E658` clamps it. */
extern "C" void fn_8011E760(_ENEMY_WORK* self, s32 value)
{
    s32 ceiling;
    s32 low;

    if (value < 0) {
        self->amount_0x7A0 = 0;
        return;
    }
    ceiling = (s32)self->field_0x7a4;
    if (value > ceiling) {
        self->amount_0x7A0 = ceiling;
        return;
    }
    low = (s32)((f32)ceiling * self->field_0x7B0);
    if (value < low) {
        self->amount_0x7A0 = low;
    } else {
        self->amount_0x7A0 = value;
    }
}

/* 0x8011E7D8 - accumulates one part's damage level. */
extern "C" void em_parts_damage_add(_ENEMY_WORK* self, u8 part, u8 add)
{
    self->parts_0x838[part].damage_level += add;
}

/* 0x8011E9DC - one part's damage level; defined at C++ scope, where `(_ENEMY_WORK*, u8)` mangles to the map's
 * name. */
u8 em_parts_damage_level_get(_ENEMY_WORK* self, u8 part)
{
    return self->parts_0x838[part].damage_level;
}

/* 0x8011F230 - OR a mask into the record's own u16 mask. */
extern "C" void fn_8011F230(_ENEMY_WORK* self, u16 mask)
{
    self->mask_0x80E |= mask;
}
