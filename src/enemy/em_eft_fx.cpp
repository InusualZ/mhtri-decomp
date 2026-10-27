/* enemy/em_eft_fx.cpp - the enemy effect family: the pooled `_EFT` creators an `_ENEMY_WORK` owns (grouped and flat
 *   work blocks), with their release, dispatch, start, move, fade and free stages.
 * RANGE. .text 0x8011D448-0x8011E5EC (20 functions); .data 0x805A0C10-0x805A0CC8 (the effect tables), .sdata
 *   0x807919C8-0x807919D0, .sdata2 0x80796C08-0x80796C10 (its 0.0f and the alignment pad), extab 0x8000C63C-0x8000C6AC,
 *   extabindex 0x80026844-0x800268EC.
 * SEAM. `.sdata2` holds 0.0f three times: 0x80796BD0 (read only by `eft030_move`, `ef/eft029_fx.cpp`), 0x80796C08
 *   (read only by this range) and 0x80796C40 (read only by `enemy/fn_8011D448.cpp`'s action script): one value pooled
 *   once per TU makes these three TUs.  The extabindex record 0x800268EC {0x8011E5EC, 0x34} opens the next unit's
 *   unwind run, and `.data` 0x805A0C10-0x805A0CC8 is read only by this range.  `ef/eft029_fx.cpp` below,
 *   `enemy/fn_8011D448.cpp` above.
 * NAMES. Every name is a GUESS from its body and callers: `em_eft_fx_*` (the `eft029_*`
 *   create/release/dispatch/start/move/state_next/free scheme; the `em_eft_fx_family_tbl` tag picks the grouped or
 *   flat body), and the data names `em_eft_fx_*_tbl`/`_ids`.  The module/file stem is a GUESS from the row names.
 * RESIDUALS.
 *  - `em_eft_fx_dispatch` (the unit's 4 B `.text` short): retail's stage-0 case is its own `b` to the inlined start
 *    body after the three tail calls; ours branches there directly.  An if-chain, `case 0` returning, the start
 *    switch inlined by hand or called with `break`s, a do/while(0) `continue`, an `if (0)` case label, a flag and
 *    `optimization_level` 1-3 all measure the same or lower.
 *  - data: the object's `.sdata2` is the 4-byte 0.0f; the claim's last 4 bytes are the alignment pad before the
 *    next unit's 8-byte-aligned double.
 * SHAPES. `#pragma peephole off`, `optimization_level 4`, `fp_contract off` and `pool_data off`, as
 *   `enemy/fn_8011D448.cpp` (the unit this range was cut from); the creators take `u32` type/timer arguments.
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
#include "enemy/fn_8012EC74.h" /* get_em_scale / em_get_mot_no (rule 2) */
#include "camera/camera.h" /* get_camera_pos (rule 2) */
#include "lobby/lb_server_sel_trans.h" /* event_demo_ck (rule 2) */
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "enemy/em_hook_get.h"
#include "enemy/em_act_trap_ck.h"
#include "enemy/em_hook9_call.h"
#include "enemy/em_motion_param_set.h"
#include "enemy/em_motion_timer_arm.h"
#include "enemy/em_mot_end_ck.h"
#include "enemy/em_pop.h"
#include "ef/fn_800CDB2C.h"
#include "ef/eft_rot_vec_copy.h"
#include "enemy/em_flag_0x8AE_ck.h"
#include "enemy/em_model.h"
#include "enemy/em025_prog.h"
#include "hud/em_net_send.h"
#include "lobby/lb_quest_screen.h"
#include "lobby/fn_8030121C.h"
#include "Pl/pl_skill.h"
#include "ef/get_move_work_adrs.h"
#include "stage/get_now_mapno.h"
#include "stage/stage_zone_hit_ck.h"
#include "sound/se.h"
#include "sound/fn_800D7F54.h"
#include "enemy/fn_80138074.h" /* em_user_data_reenter (rule 2) */

/* The unit's codegen pragmas (the unit header's SHAPES line). */
#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off
#pragma pool_data off

/* This unit's own `.data` (0x805A0C10-0x805A0CC8) and `.sdata` (0x807919C8-0x807919D0). */
extern "C" {
/* [type * 2 + group] effect id of the grouped family for the default owner kind. */
u16 em_eft_fx_group_ids[8] = { 0x119, 0x11A, 0x11B, 0x11C, 0x11D, 0x11E, 0x143, 0x143 };
/* [type - 4] effect id of the flat family. */
u16 em_eft_fx_flat_ids[8] = { 0x1AB, 0x144, 0x1C1, 0x1FD, 0x26A, 0x269, 0x1FD, 0x1FD };
/* The grouped ids for owner kinds 15, 16 and 18. */
u16 em_eft_fx_group_ids_em15[8] = { 0x4D7, 0x4D8, 0x4D9, 0x4DA, 0x4DB, 0x4DC, 0x4DD, 0x4DD };
u16 em_eft_fx_group_ids_em16[8] = { 0xA2B, 0xA2C, 0x48D, 0x48E, 0xA2D, 0xA2E, 0x48F, 0x490 };
u16 em_eft_fx_group_ids_em18[8] = { 0xA31, 0xA32, 0x11B, 0x11C, 0xA33, 0xA34, 0x143, 0x143 };
/* [type] effect count the flat-family creators store. */
u8 em_eft_fx_flat_count_tbl[16] = { 15, 15, 15, 15, 1, 1, 1, 2, 1, 1, 2, 1 };
/* [type] resource group of the flat family. */
u16 em_eft_fx_flat_group_tbl[12] = { 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x26, 0x24, 0x24, 0x26, 0x26 };
/* [type] offset z of the target vector. */
f32 em_eft_fx_offset_z_tbl[12] = { 0.0f, 0.0f, 0.0f, 100.0f, 100.0f, 0.0f, 800.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
/* [type] family tag (0 = grouped, 1 = flat). */
u8 em_eft_fx_family_tbl[16] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 };
/* [type * 2 + group] re-arm delay of a grouped slot. */
s8 em_eft_fx_group_delay_tbl[8] = { 4, 4, 2, 2, 7, 7, 5, 0x50 };
}

/* This unit's own members, used before their definitions. */
extern "C" {
void em_eft_fx_start_groups(_EFT* self);
void em_eft_fx_start_flat(_EFT* self);
void em_eft_fx_move(_EFT* self);
void em_eft_fx_move_groups(_EFT* self);
void em_eft_fx_move_flat(_EFT* self);
void em_eft_fx_fade(_EFT* self);
void em_eft_fx_fade_groups(_EFT* self);
void em_eft_fx_state_next(_EFT* self);
void em_eft_fx_free(_EFT* self);
void em_eft_fx_place_at_joint(_EFT* self, nw4r::ef::Effect* effect, u8 group);
void em_eft_fx_release_groups(_EFT* self);
void em_eft_fx_release_flat(_EFT* self);
void em_eft_fx_release(_EFT* self);
void em_eft_fx_dispatch(_EFT* self);
_EFT* em_eft_fx_create(_ENEMY_WORK* self, u32 kind, u32 id0, u32 id1, f32 scale, u8 timer);
}

/* The effect record's per-family work blocks. */

/* One group of the grouped family: the live slots and the countdown to the next re-arm.  size: 0x44 */
struct EmEftFxGroup {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[15];
    /* +0x40 */ s8 delay;
    /* +0x41 */ u8 pad_0x41[3];
};

/* The grouped family's work block (the `0xA8` pool `em_eft_fx_create` asks for): two groups placed at the two
 * joints the creator passes, the scale, the offset the joints are pushed along, and the owner's motion the
 * effect lives for.  size: 0xA8 */
struct EmEftFxGroupWork {
    /* +0x00 */ s32 group_count;
    /* +0x04 */ EmEftFxGroup groups[2];
    /* +0x8C */ f32 scale;
    /* +0x90 */ u32 joints[2];
    /* +0x98 */ nw4r::math::VEC3 offset;
    /* +0xA4 */ u16 mot_no;
    /* +0xA6 */ u8 pad_0xA6[2];
};

/* The flat family's work block (the `0x20` pool): up to two effects at one joint.  size: 0x20 */
struct EmEftFxFlatWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[2];
    /* +0x0C */ f32 scale;
    /* +0x10 */ u32 joint;
    /* +0x14 */ nw4r::math::VEC3 pos;
};

/* Bodies. */

/* 0x8011D448 - spawns one grouped effect and points its offset along the type table's z. */
extern "C" void em_eft_fx_spawn(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, f32 scale)
{
    _EFT* effect = em_eft_fx_create(self, type, id, id, scale, timer);
    EmEftFxGroupWork* work;

    if (effect != NULL) {
        work = (EmEftFxGroupWork*)effect->work_0x38;
        setVector3(&work->offset, 0.0f, 0.0f, em_eft_fx_offset_z_tbl[type]);
    }
}

/* 0x8011D4AC - the same spawn, with the offset copied from the caller's vector. */
extern "C" void em_eft_fx_spawn_at(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, const nw4r::math::VEC3* pos,
                                   f32 scale)
{
    _EFT* effect = em_eft_fx_create(self, type, id, id, scale, timer);
    EmEftFxGroupWork* work;

    if (effect != NULL) {
        work = (EmEftFxGroupWork*)effect->work_0x38;
        copyVec3(&work->offset, pos);
    }
}

/* 0x8011D4FC - the same spawn with two joints. */
extern "C" void em_eft_fx_spawn_pair(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, u32 timer, f32 scale)
{
    _EFT* effect = em_eft_fx_create(self, type, id0, id1, scale, timer);
    EmEftFxGroupWork* work;

    if (effect != NULL) {
        work = (EmEftFxGroupWork*)effect->work_0x38;
        setVector3(&work->offset, 0.0f, 0.0f, em_eft_fx_offset_z_tbl[type]);
    }
}

/* 0x8011D558 - the grouped family's creator: within the enemy's own area, fills the work block and stamps the
 * record (tag 0x1f, type, timer, owner, area, rotation), installs the release/dispatch pair, marks demo ownership. */
extern "C" _EFT* em_eft_fx_create(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, f32 scale, u8 timer)
{
    _EFT* effect;
    EmEftFxGroupWork* work;

    if (get_now_areano() != self->act_id) {
        return NULL;
    }

    effect = (_EFT*)eft_res_slot_get(0xA8);
    if (effect == NULL) {
        return NULL;
    }

    work = (EmEftFxGroupWork*)effect->work_0x38;
    work->group_count = 2;
    work->scale = scale * get_em_scale(self);
    work->joints[0] = id0;
    work->joints[1] = id1;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->timer_0x0C = timer;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    effect->release_0x40 = em_eft_fx_release;
    effect->dispatch_0x34 = em_eft_fx_dispatch;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    return effect;
}

/* 0x8011D690 - the flat family's creator on the enemy's own joint. */
extern "C" void em_eft_fx_spawn_anchor(_ENEMY_WORK* self, u32 type, u32 joint, f32 scale)
{
    _EFT* effect;
    EmEftFxFlatWork* work;

    if (get_now_areano() != self->act_id) {
        return;
    }

    effect = (_EFT*)eft_res_slot_get(0x20);
    if (effect == NULL) {
        return;
    }

    work = (EmEftFxFlatWork*)effect->work_0x38;
    work->count = em_eft_fx_flat_count_tbl[type];
    work->scale = scale * get_em_scale(self);
    work->joint = joint;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    effect->timer_0x0C = 0;
    eft_state_flags_set(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = em_eft_fx_release;
    effect->dispatch_0x34 = em_eft_fx_dispatch;
}

/* 0x8011D7B0 - the flat family's creator that belongs to nobody: the caller passes the type, position, rotation
 * and area, so `source_0x30` stays NULL. */
extern "C" void em_eft_fx_spawn_free(u32 type, const nw4r::math::VEC3* pos, const _CP_VECTOR* rot, f32 scale,
                                     u8 area, u8 demo)
{
    _EFT* effect;
    EmEftFxFlatWork* work;

    if (get_now_areano() != area) {
        return;
    }

    effect = (_EFT*)eft_res_slot_get(0x20);
    if (effect == NULL) {
        return;
    }

    work = (EmEftFxFlatWork*)effect->work_0x38;
    work->count = em_eft_fx_flat_count_tbl[type];
    work->scale = scale;

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
    effect->release_0x40 = em_eft_fx_release;
    effect->dispatch_0x34 = em_eft_fx_dispatch;
}

/* 0x8011D8C0 - the family's `.release` callback: hands the type's slots back. */
extern "C" void em_eft_fx_release(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_release_groups(self);
        break;
    case 1:
        em_eft_fx_release_flat(self);
        break;
    }
}

/* 0x8011D8F0 - the grouped release: every slot of every group, one call each, then the group count is cleared. */
extern "C" void em_eft_fx_release_groups(_EFT* self)
{
    EmEftFxGroupWork* work = (EmEftFxGroupWork*)self->work_0x38;
    s32 i;
    s32 j;

    for (i = 0; i < work->group_count; i++) {
        for (j = 0; j < work->groups[i].count; j++) {
            push_eft_effect_heap_num(&work->groups[i].slots[j], 1);
        }
    }
    work->group_count = 0;
}

/* 0x8011D97C - the flat release: the whole run in one call, then the count is cleared. */
extern "C" void em_eft_fx_release_flat(_EFT* self)
{
    EmEftFxFlatWork* work = (EmEftFxFlatWork*)self->work_0x38;

    push_eft_effect_heap_num(work->slots, work->count);
    work->count = 0;
}

/* The stage-0 body: creates the type's effects. */
static inline void em_eft_fx_start(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_start_groups(self);
        break;
    case 1:
        em_eft_fx_start_flat(self);
        break;
    }
}

/* 0x8011D9B8 - the family's `.dispatch` callback: `state_0x05` selects the stage. */
extern "C" void em_eft_fx_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        em_eft_fx_move(self);
        return;
    case 2:
        em_eft_fx_fade(self);
        return;
    case 3:
        em_eft_fx_free(self);
        return;
    default:
        return;
    }
    em_eft_fx_start(self);
}

/* 0x8011DA24 - the grouped start: one effect per group at its joint, the owner's motion latched, then the first
 * move. */
extern "C" void em_eft_fx_start_groups(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    EmEftFxGroupWork* work;
    _ENEMY_WORK* owner;
    s32 i;
    u16* ids;
    u16 group_id;

    MTX34_ctor(&mtx);
    work = (EmEftFxGroupWork*)self->work_0x38;
    owner = (_ENEMY_WORK*)self->source_0x30;
    self->state_0x05++;
    switch (self->demo_flag_0x08) {
    case 15:
        ids = em_eft_fx_group_ids_em15;
        group_id = 0x26;
        break;
    case 16:
        ids = em_eft_fx_group_ids_em16;
        group_id = 0x4C;
        break;
    case 18:
        ids = em_eft_fx_group_ids_em18;
        group_id = 0x45;
        break;
    default:
        ids = em_eft_fx_group_ids;
        group_id = 0x24;
        break;
    }
    for (i = 0; i < work->group_count; i++) {
        work->groups[i].slots[0] = res_eft_create(ids[i + self->type_0x02 * 2], group_id, 0);
        if (work->groups[i].slots[0] == NULL) {
            em_eft_fx_free(self);
            return;
        }
        em_eft_fx_place_at_joint(self, work->groups[i].slots[0], i);
        work->groups[i].count++;
        work->groups[i].delay = em_eft_fx_group_delay_tbl[i + self->type_0x02 * 2];
    }
    work->mot_no = em_get_mot_no(owner);
    self->flag_0x01 = 1;
    em_eft_fx_move(self);
}

/* 0x8011DB74 - the flat start: the type's effect (an owner-kind variant for types 5/6/8/9), a second one for
 * types 7/10, all placed at the joint pushed along the type's offset. */
extern "C" void em_eft_fx_start_flat(_EFT* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    EmEftFxFlatWork* work;
    _ENEMY_WORK* owner;
    u16 id;
    u16 group_id;
    s32 i;

    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);
    work = (EmEftFxFlatWork*)self->work_0x38;
    owner = (_ENEMY_WORK*)self->source_0x30;
    self->state_0x05++;
    id = em_eft_fx_flat_ids[self->type_0x02 - 4];
    group_id = em_eft_fx_flat_group_tbl[self->type_0x02];
    switch (self->type_0x02) {
    case 5:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4DE;
            group_id = 0x26;
            break;
        }
        break;
    case 6:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4EE;
            group_id = 0x26;
            break;
        case 20:
            id = 0x89E;
            group_id = 0x70;
            break;
        }
        break;
    case 8:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4D1;
            group_id = 0x26;
            break;
        case 16:
            id = 0xA30;
            group_id = 0x4C;
            break;
        case 20:
            id = 0x895;
            group_id = 0x70;
            break;
        }
        break;
    case 9:
        switch (self->demo_flag_0x08) {
        case 15:
            id = 0x4D0;
            group_id = 0x26;
            break;
        case 16:
            id = 0xA2F;
            group_id = 0x4C;
            break;
        case 20:
            id = 0x894;
            group_id = 0x70;
            break;
        }
        break;
    }
    work->slots[0] = res_eft_create(id, group_id, 0);
    if (work->slots[0] == NULL) {
        em_eft_fx_free(self);
        return;
    }
    switch (self->type_0x02) {
    case 7:
    case 10:
        work->slots[1] = res_eft_create(0x3B8, 0x26, 0);
        if (work->slots[1] == NULL) {
            em_eft_fx_free(self);
            return;
        }
        break;
    }
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    setVector3(&offset, 0.0f, 0.0f, em_eft_fx_offset_z_tbl[self->type_0x02]);
    mulVecMat(&offset, &mtx);
    if (owner != NULL) {
        if (em_work_die_ck(owner) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        get_joint_wpos_em(owner, work->joint, &self->pos_0x18);
        addVec3To(&self->pos_0x18, &offset);
        copyVec3(&work->pos, &self->pos_0x18);
    }
    mtx34_set_trans(&mtx, &self->pos_0x18);
    for (i = 0; i < work->count; i++) {
        work->slots[i]->SetRootMtx(mtx);
    }
    self->flag_0x01 = 1;
    em_eft_fx_move(self);
}

/* 0x8011DE2C - the stage-1 body. */
extern "C" void em_eft_fx_move(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_move_groups(self);
        break;
    case 1:
        em_eft_fx_move_flat(self);
        break;
    }
}

/* Copies the stage's ambient colour onto one pooled effect. */
static inline void em_eft_fx_stage_color(_GXColor* color, u8 area)
{
    u32 rgba = get_stg_eft_col(area, 1);

    color->r = (rgba & 0xFF000000) >> 24;
    color->g = (rgba & 0xFF0000) >> 16;
    color->b = (rgba & 0xFF00) >> 8;
    color->a = rgba;
}

/* 0x8011DE5C - the grouped move: while the owner lives and keeps its motion, every group re-arms one slot per
 * delay, every slot is scaled and stepped, and a live slot under water takes the stage colour. */
extern "C" void em_eft_fx_move_groups(_EFT* self)
{
    s32 i;
    s32 j;
    EmEftFxGroupWork* work;
    _ENEMY_WORK* owner;
    u16* ids;
    u16 group_id;
    _GXColor color;

    work = (EmEftFxGroupWork*)self->work_0x38;
    owner = (_ENEMY_WORK*)self->source_0x30;
    if (em_work_die_ck(owner) != 0) {
        self->state_0x05++;
        return;
    }
    if (self->field_0x07 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->timer_0x0C = 0;
        self->state_0x05++;
        return;
    }
    if (--self->timer_0x0C < 0 || work->mot_no != em_get_mot_no(owner)) {
        self->state_0x05++;
        self->timer_0x0C = 60;
        em_eft_fx_fade(self);
        return;
    }
    switch (self->demo_flag_0x08) {
    case 15:
        ids = em_eft_fx_group_ids_em15;
        group_id = 0x26;
        break;
    case 16:
        ids = em_eft_fx_group_ids_em16;
        group_id = 0x4C;
        break;
    case 18:
        ids = em_eft_fx_group_ids_em18;
        group_id = 0x45;
        break;
    default:
        ids = em_eft_fx_group_ids;
        group_id = 0x24;
        break;
    }
    for (i = 0; i < work->group_count; i++) {
        work->groups[i].delay--;
        if (work->groups[i].delay < 0 && work->groups[i].count < 15) {
            work->groups[i].slots[work->groups[i].count] = res_eft_create(ids[i + self->type_0x02 * 2], group_id, 0);
            if (work->groups[i].slots[work->groups[i].count] != NULL) {
                em_eft_fx_place_at_joint(self, work->groups[i].slots[work->groups[i].count], i);
                work->groups[i].count++;
                work->groups[i].delay = em_eft_fx_group_delay_tbl[i + self->type_0x02 * 2];
            }
        }
        for (j = 0; j < work->groups[i].count; j++) {
            change_paramscale_eff(work->groups[i].slots[j], work->scale);
            if (effect_move(work->groups[i].slots[j]) == 1) {
                if (get_camera_pos().y < 0.0f) {
                    em_eft_fx_stage_color(&color, self->area_0x44);
                    change_color_eff(work->groups[i].slots[j], &self->pos_0x18, color);
                    eft_res_models_spawn(self, (void**)&work->groups[i].slots[j], 1, 1, NULL);
                }
            }
        }
    }
}

/* 0x8011E138 - the flat move: types 5 and 8-10 follow the owner's joint, every effect is scaled and stepped
 * (one that ends closes the stage), and under water the stage colour and the model pass are applied. */
extern "C" void em_eft_fx_move_flat(_EFT* self)
{
    EmEftFxFlatWork* work;
    _ENEMY_WORK* owner;
    s32 i;
    _GXColor color;

    work = (EmEftFxFlatWork*)self->work_0x38;
    if (self->field_0x07 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if ((u32)(self->type_0x02 - 8) <= 2 || (s32)self->type_0x02 == 5) {
        owner = (_ENEMY_WORK*)self->source_0x30;
        if (em_work_die_ck(owner) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            self->timer_0x0C = 0;
            return;
        }
        get_joint_wpos_em(owner, work->joint, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->slots[i], &self->pos_0x18);
        }
    }
    for (i = 0; i < work->count; i++) {
        change_paramscale_eff(work->slots[i], work->scale);
        if (effect_move(work->slots[i]) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            self->timer_0x0C = 0;
            return;
        }
    }
    if (get_camera_pos().y < 0.0f) {
        em_eft_fx_stage_color(&color, self->area_0x44);
        for (i = 0; i < work->count; i++) {
            change_color_eff(work->slots[i], &self->pos_0x18, color);
        }
        if ((u32)(self->type_0x02 - 8) <= 1) {
            eft_res_models_spawn(self, (void**)&work->slots[0], 1, 1, &work->pos);
        } else {
            eft_res_models_spawn(self, (void**)&work->slots[0], 1, 1, NULL);
        }
    }
    switch (self->type_0x02) {
    case 7:
    case 10:
        eft_res_models_spawn(self, (void**)&work->slots[1], 1, 1, NULL);
        break;
    }
}

/* 0x8011E378 - the stage-2 body. */
extern "C" void em_eft_fx_fade(_EFT* self)
{
    switch (em_eft_fx_family_tbl[self->type_0x02]) {
    case 0:
        em_eft_fx_fade_groups(self);
        break;
    case 1:
        em_eft_fx_state_next(self);
        break;
    }
}

/* 0x8011E3A8 - the grouped fade: the live slots run out their timer (stage colour under water), then the stage
 * ends. */
extern "C" void em_eft_fx_fade_groups(_EFT* self)
{
    s32 i;
    s32 j;
    EmEftFxGroupWork* work;
    _GXColor color;

    work = (EmEftFxGroupWork*)self->work_0x38;
    switch (self->field_0x06) {
    case 0:
        self->field_0x06++;
    case 1:
        if (--self->timer_0x0C >= 0) {
            for (i = 0; i < work->group_count; i++) {
                for (j = 0; j < work->groups[i].count; j++) {
                    if (effect_move(work->groups[i].slots[j]) == 1) {
                        if (get_camera_pos().y < 0.0f) {
                            em_eft_fx_stage_color(&color, self->area_0x44);
                            change_color_eff(work->groups[i].slots[j], &self->pos_0x18, color);
                            eft_res_models_spawn(self, (void**)&work->groups[i].slots[j], 1, 1, NULL);
                        }
                    }
                }
            }
        } else {
            self->state_0x05++;
        }
        break;
    }
}

/* 0x8011E51C - closes the current stage. */
extern "C" void em_eft_fx_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011E52C - the terminal stage: hands the whole pooled record back. */
extern "C" void em_eft_fx_free(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x8011E530 - places `effect` at the owner's joint for `group`, pushed along the work's offset turned by the
 * record's rotation. */
extern "C" void em_eft_fx_place_at_joint(_EFT* self, nw4r::ef::Effect* effect, u8 group)
{
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    _ENEMY_WORK* owner;
    EmEftFxGroupWork* work;

    VEC3_ctor(&joint);
    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);

    owner = (_ENEMY_WORK*)self->source_0x30;
    work = (EmEftFxGroupWork*)self->work_0x38;

    cpSetRotMatrix(&self->rot_0x24, &mtx);
    copyVec3(&offset, &work->offset);
    mulVecMat(&offset, &mtx);
    get_joint_wpos_em(owner, work->joints[group], &joint);
    addVec3To(&joint, &offset);
    mtx34_set_trans(&mtx, &joint);
    effect->SetRootMtx(mtx);
}
