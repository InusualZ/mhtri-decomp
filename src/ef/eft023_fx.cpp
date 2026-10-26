/* ef/eft023_fx.cpp - the eft023 and eft024 job machines.  Each creator takes the 0x48-byte `_EFT` record with a work
 *   block from `eft_res_slot_get`, stamps `+0x03` and installs its hooks: 23 `eft023_area_fx_set` (0x4C `_EFT_JOB_WORK`,
 *   release `fn_8011688C`, dispatch `fn_8011689C`), 24 `eft023_area_fx2_set` (0x84 `_EFT_JOB2_WORK`, `fn_80116FCC`,
 *   `fn_80116FDC`).
 * RANGE. .text 0x80116770-0x8011722C (19 functions); extab 0x8000C464-0x8000C494, extabindex 0x80026580-0x800265C8,
 *   .data 0x805A0380-0x805A0460, .sdata2 0x80796A60-0x80796A80.  A TU of its own: its `.sdata2` pool repeats
 *   `ef/eft022_fx.cpp`'s 1.0f, -1.0f, 0.5f and int-to-float magic (0x80796A60..0x80796A7C, read by `fn_801168D8`/
 *   `fn_80116B00`), its `.data` 0x805A0380-0x805A0460 is read only by `fn_801168D8`, and extabindex 0x80026580 opens on
 *   `eft023_area_fx_set`.  `ef/fn_8011722C.c` follows.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (playbook 39) and `#pragma unroll off` around
 *   `fn_80117120`.
 * NAMES. The file name is a GUESS (the next family number after `ef/eft022_fx.cpp`).  The map has only `fn_` stems,
 *   so plain definitions are `extern "C"`.  GUESS: `eft024_job_start`/`eft024_job_step` (the file-local state-0/1
 *   bodies at 0x80117018/0x80117050).
 * RESIDUALS.
 *  - `fn_80116B00`: the int-to-float conversion is unsigned here (`xoris` magic `0x43300000 00000000`), signed in
 *    retail;
 *  - `fn_80116EEC`: retail unrolls the slot scan with the table base in r3; ours keeps a separate base;
 *  - `fn_80117120`: retail keeps the dead loop counter's increment in both unrolled halves.
 *   `.data` is declared, not emitted.  Relocation names that differ from retail: pool constants.
 * SHAPES. Functions are defined in retail's address order.  `fn_80116FDC` tail-calls two static functions for states 0
 *   and 1 (retail's `b` into the folded bodies after the exit `blr`).  `fn_801168D8` writes area 1/2 for map 6 as
 *   if/else and for map 17 as a ternary.  `fn_80116EEC` nests `for (j = 0; j < 4; j++) for (k = 0; k < 8; k++)`.
 *   `fn_80116B00` loads its seven float constants into named locals before the loop.
 */

#include "ef/eft_res_model_get.h" /* eft_res_model_get (rule 2: the owner's header) */
#include "ef/eft_res_slot_get.h" /* eft_res_slot_get (rule 2: the owner's header) */
#include "ef/eft_res_slot_release.h" /* eft_res_slot_release (rule 2: the owner's header) */
#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "ef/eft_res_spawn_gate_ck.h" /* eft_res_spawn_gate_ck (rule 2: the owner's header) */
#include "ef/eft_res_models_spawn.h" /* eft_res_models_spawn (rule 2: the owner's header) */
#include "ef/mtx34_trans_get.h" /* mtx34_trans_get (rule 2: the owner's header) */
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
#include "g3d/g3d_scnmdlsmpl.h" /* fn_80080B10/fn_800810DC (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_8006FDCC (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "unsplit/sound.h"
#include "unsplit/Pl.h"
#include "unsplit/ef.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_80114E34_types.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_res_model_get_c1 ((void* (*)(void))eft_res_model_get)
#define eft_res_slot_get_c1 ((_EFT* (*)(u32))eft_res_slot_get)
#define eft_state_flags_set_c1 ((void (*)(_EFT*, u8, u8))eft_state_flags_set)
#define eft_res_models_spawn_c1 ((void (*)(void*, void*, u32, s32, u32))eft_res_models_spawn)

#include "MTX/mtx.h" /* Vec (rule 1) */

/* One entry of the shared spawn-offset tables the eft023 models are placed from. size: 0x1C */
struct _EFT_MAP_TABLE {
    /* +0x00 */ u8 unused_0x00[0x04];
    /* +0x04 */ nw4r::math::VEC3 pos_0x04;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 field_0x11;
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ f32 field_0x14;
    /* +0x18 */ u8 field_0x18;
    /* +0x19 */ u8 unused_0x19;
    /* +0x1A */ s16 field_0x1A;
    /* +0x1C */ u8 unused_0x1C[0x1C - 0x1C];
};

/* The eft023 work block. size: 0x4C */
struct _EFT_JOB_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[5];
    /* +0x18 */ s16 field_0x18[5];
    /* +0x22 */ s16 field_0x22[5];
    /* +0x2C */ s16 field_0x2C[5];
    /* +0x38 */ _EFT_MAP_TABLE* tables[5];
};

/* The eft024 work block: 32 character slots and the live counters. size: 0x84 */
struct _EFT_JOB2_WORK {
    /* +0x00 */ _EFT* slots[32];
    /* +0x80 */ s16 total;
    /* +0x82 */ s16 per_type[1];
};

extern "C" void eft023_area_fx_set(void);
extern "C" void fn_8011688C(_EFT*);
extern "C" void fn_8011689C(_EFT*);
extern "C" void fn_801168D8(_EFT*);
extern "C" void fn_80116B00(_EFT*);
extern "C" void fn_80116D90(_EFT*);
extern "C" void fn_80116DA0(_EFT*);
extern "C" void eft023_area_fx2_set(void);
extern "C" void* fn_80116E14(s16);
extern "C" void fn_80116E1C(_EFT*, s16, s32);
extern "C" s16 fn_80116EEC(_EFT*);
extern "C" void fn_80116FCC(_EFT*);
extern "C" void fn_80116FDC(_EFT*);
extern "C" void fn_80117074(_EFT*);
extern "C" void fn_80117084(_EFT*);
extern "C" void fn_80117088(_EFT*);
extern "C" void fn_80117120(_EFT*);

/* The library entry points, declared by their map spelling (a mangled callee is not evidence of a
 * C++ unit; only a mangled definition is). */
extern "C" f32 sinf(s16 a, f32 b);

extern "C" _EFT_MAP_TABLE lbl_805A0380[];
extern "C" _EFT_MAP_TABLE lbl_805A039C[];
extern "C" _EFT_MAP_TABLE lbl_805A0428[];

extern "C" f64 lbl_80796A78;

#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * eft023 (fn_8011688C release, fn_8011689C dispatch)
 * --------------------------------------------------------------------------------------------------- */

/* Creates the eft023 record and its per-area model set. */
extern "C" void eft023_area_fx_set(void)
{
    _EFT* effect;
    _EFT_JOB_WORK* work;
    s32 i;
    s32 map;
    s32 id;

    effect = eft_res_slot_get_c1(0x4C);
    if (effect == 0) {
        return;
    }
    effect->release_0x40 = fn_8011688C;
    work = (_EFT_JOB_WORK*)effect->work_0x38;
    work->count = 1;
    effect->area_0x44 = get_now_areano();
    map = get_now_mapno();
    if (map == 6 || map == 0x11) {
        if (effect->area_0x44 == 1) {
            work->count = 5;
        } else if (effect->area_0x44 == 2) {
            work->count = 2;
        }
    }
    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get_c1();
        if (work->models[i] == 0) {
            eft_res_slot_release(effect);
            return;
        }
    }
    effect->source_0x30 = 0;
    effect->dispatch_0x34 = fn_8011689C;
    effect->field_0x03 = 23;
    eft_state_flags_set_c1(effect, 8, 0);
}

/* Releases the eft023 record's character handles. */
extern "C" void fn_8011688C(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;

    eft_res_models_release((void**)work->models, work->count);
}

/* Runs the state handler the eft023 record's `state_0x05` selects. */
extern "C" void fn_8011689C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_801168D8(self);
    case 1:
        return fn_80116B00(self);
    case 2:
        return fn_80116D90(self);
    case 3:
        return fn_80116DA0(self);
    }
}

/* Spawns the eft023 record's models for the current map and area. */
extern "C" void fn_801168D8(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;
    s32 i;
    u8 map;
    s32 id;

    self->state_0x05++;
    map = get_now_mapno();
    switch (map) {
    case 1:
        id = 0x11;
        break;
    case 12:
        id = 0x12;
        break;
    case 6:
        if (self->area_0x44 == 1) {
            id = 0x13;
        } else {
            id = 0x15;
        }
        break;
    case 17:
        id = (self->area_0x44 == 1) ? 0x14 : 0x16;
        break;
    }
    for (i = 0; i < work->count; i++) {
        switch (get_now_mapno()) {
        case 1:
        case 12:
            if (self->area_0x44 == 1) {
                work->tables[i] = &lbl_805A0380[i];
            }
            break;
        case 6:
        case 17:
            if (self->area_0x44 == 1) {
                work->tables[i] = &lbl_805A039C[i];
            } else if (self->area_0x44 == 2) {
                work->tables[i] = &lbl_805A0428[i];
            }
            break;
        }
        if (res_eft_model_create_light(work->models[i], id, 0x28, 2) == 0) {
            fn_80116DA0(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    self->field_0x10 = 0;
    for (i = 0; i < work->count; i++) {
        vec_to_mh_vec3(&work->models[i]->pos_0x04, (Vec*)&work->tables[i]->pos_0x04);
        setVector3(&work->models[i]->scale_0x1C, 1.0f, 1.0f,
                                           1.0f);
        work->models[i]->field_0x28 = 0;
        work->models[i]->field_0x2C = work->tables[i]->field_0x12;
        work->models[i]->field_0x28 = 0;
        work->models[i]->ready_0x35 = 0;
        work->field_0x18[i] = 0;
        work->field_0x22[i] = (s16)(work->tables[i]->field_0x10 - 1);
        work->field_0x2C[i] = 0;
    }
    fn_80116B00(self);
}

/* Advances the eft023 record's per-model joint visibility and rotation. */
extern "C" void fn_80116B00(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;
    _EFT_MAP_TABLE* table;
    s32 i;
    u32 j;
    u32 joint_num;
    f32 f;
    s16 t;
    u8 base;
    f32 f25 = lbl_80796A78;
    f32 f26 = 6.2831855f;
    f32 f27 = 65536.0f;
    f32 f28 = 1.0f;
    f32 f29 = -1.0f;
    f32 f30 = 360.0f;
    f32 f31 = 0.5f;

    for (i = 0; i < work->count; i++) {
        table = work->tables[i];
        work->field_0x22[i] = (s16)(work->field_0x22[i] + 1);
        base = table->field_0x10;
        if (work->field_0x22[i] >= (s32)(base + table->field_0x11)) {
            work->field_0x22[i] = base;
        }
        joint_num = work->models[i]->get_joint_num();
        for (j = 0; j < joint_num; j++) {
            if ((s32)j == work->field_0x22[i]) {
                work->models[i]->setVisibility(j, 1);
            } else {
                work->models[i]->setVisibility(j, 0);
            }
        }
        self->field_0x10++;
        work->field_0x18[i] = (s16)(work->field_0x18[i] + 1);
        t = work->field_0x18[i];
        f = sinf(t, (f26 * (f32)(u16)(t << table->field_0x18)) / f27);
        if ((f >= f28 || f <= f29) && table->field_0x1A > 0) {
            work->field_0x2C[i] = (s16)(work->field_0x2C[i] + 1);
            if (work->field_0x2C[i] > table->field_0x1A) {
                work->field_0x2C[i] = 0;
            } else {
                work->field_0x18[i] = (s16)(work->field_0x18[i] - 1);
            }
        }
        work->models[i]->field_0x2C =
            (s32)(u16)(s32)(f31 + ((f27 * (f * table->field_0x14)) / f30)) + table->field_0x12;
        work->models[i]->move(0);
        eft_res_models_spawn_c1(self, &work->models[i], 2, 1, 0);
    }
}

/* Bumps the eft023 record's `state_0x05` state index. */
extern "C" void fn_80116D90(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the eft023 record: the family-23 state-3 step. */
extern "C" void fn_80116DA0(_EFT* self)
{
    eft_res_slot_release(self);
}

/* ---------------------------------------------------------------------------------------------------
 * eft024 (fn_80116FCC release, fn_80116FDC dispatch)
 * --------------------------------------------------------------------------------------------------- */

/* Creates the eft024 job record. */
extern "C" void eft023_area_fx2_set(void)
{
    _EFT* effect = eft_res_slot_get_c1(0x84);

    if (effect != 0) {
        effect->source_0x30 = 0;
        effect->dispatch_0x34 = fn_80116FDC;
        effect->field_0x03 = 24;
        eft_state_flags_set_c1(effect, 8, 0);
        effect->flag_0x01 = 1;
        effect->area_0x44 = get_now_areano();
    }
}

/* Allocates one eft024 character slot. */
extern "C" void* fn_80116E14(s16 unused)
{
    return eft_res_slot_get_c1(0x20);
}

/* Adds one character to the eft024 job's free slot. */
extern "C" void fn_80116E1C(_EFT* self, s16 type, s32 unused_arg)
{
    _EFT_JOB2_WORK* work = (_EFT_JOB2_WORK*)self->work_0x38;
    s16 slot;
    _EFT* effect;

    slot = fn_80116EEC(self);
    if (slot == -1) {
        return;
    }
    effect = (_EFT*)fn_80116E14(type);
    if (effect == 0) {
        return;
    }
    effect->field_0x03 = 24;
    effect->type_0x02 = (u8)type;
    effect->dispatch_0x34 = fn_80116FDC;
    eft_state_flags_set_c1(effect, 8, 0);
    effect->area_0x44 = get_now_areano();
    work->slots[slot] = effect;
    work->per_type[type - 1]++;
    work->total++;
}

/* Finds the first free slot in the eft024 job's 32-slot table. */
extern "C" s16 fn_80116EEC(_EFT* self)
{
    s16 i = 0;
    _EFT** p = (_EFT**)self->work_0x38;
    s32 j;
    s32 k;

    for (j = 0; j < 4; j++) {
        for (k = 0; k < 8; k++) {
            if (p[k] == 0) {
                return i;
            }
            i++;
        }
        p += 8;
    }
    return -1;
}

/* Releases the eft024 job's character handles. */
extern "C" void fn_80116FCC(_EFT* self)
{
    _EFT_JOB_WORK* work = (_EFT_JOB_WORK*)self->work_0x38;

    eft_res_models_release((void**)work->models, work->count);
}

static void eft024_job_start(_EFT* self);
static void eft024_job_step(_EFT* self);

/* Runs the state handler the eft024 job's `state_0x05` selects. */
extern "C" void fn_80116FDC(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft024_job_start(self);
        break;
    case 1:
        eft024_job_step(self);
        break;
    case 2:
        fn_80117074(self);
        break;
    case 3:
        fn_80117084(self);
        break;
    }
}

/* State 0: advances the job and runs the kind's start.  File-local, so the DOL map folds it into the dispatcher's
 * extent. */
static void eft024_job_start(_EFT* self)
{
    self->state_0x05++;
    self->flag_0x01 = 1;
    switch (self->type_0x02) {
    case 0:
        fn_80117088(self);
        break;
    case 1:
        fn_8011722C(self);
        break;
    }
}

/* State 1: runs the kind's per-frame step.  File-local, folded into the dispatcher's extent like the start. */
static void eft024_job_step(_EFT* self)
{
    switch (self->type_0x02) {
    case 0:
        fn_80117120(self);
        break;
    case 1:
        fn_801173AC(self);
        break;
    }
}

/* Bumps the eft024 job's `state_0x05` state index. */
extern "C" void fn_80117074(_EFT* self)
{
    self->state_0x05++;
}

/* Releases the eft024 job: the family-24 state-3 step. */
extern "C" void fn_80117084(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Zeroes the eft024 job's slot table and counters. */
extern "C" void fn_80117088(_EFT* self)
{
    _EFT_JOB2_WORK* work = (_EFT_JOB2_WORK*)self->work_0x38;
    s32 i;

    self->timer_0x0C = 0;
    for (i = 0; i < 32; i++) {
        work->slots[i] = 0;
    }
    work->total = 0;
    work->per_type[0] = 0;
}

#pragma unroll off

/* Spawns a new character every second while the eft024 job's table has room, and reaps dead ones. */
extern "C" void fn_80117120(_EFT* self)
{
    _EFT_JOB2_WORK* work = (_EFT_JOB2_WORK*)self->work_0x38;
    _EFT* handle;
    s32 i;

    if (++self->timer_0x0C >= 0x3C && work->total < 0x20) {
        fn_80116E1C(self, 1, 0);
        self->timer_0x0C = 0;
    }
    for (i = 0; i < 0x20; i++) {
        handle = work->slots[i];
        if (handle != 0 && handle->state_0x05 >= 2) {
            work->per_type[handle->type_0x02 - 1]--;
            work->slots[i] = 0;
            work->total--;
        }
    }
}

#pragma unroll reset

