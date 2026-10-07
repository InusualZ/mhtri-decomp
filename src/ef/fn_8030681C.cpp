/* ef/fn_8030681C.cpp - the eft041/eft042 effect family's state machine and the two status-screen entry points
 *   below `menu/menu_infomation.cpp`.
 * RANGE. .text 0x8030681C-0x80308FB4 (26 functions); extab 0x80015AB4-0x80015B54, extabindex 0x80034674-0x80034764,
 *   .data 0x805DC5E8-0x805DCC50, .sdata 0x80792B78-0x80792BB8, .sdata2 0x8079ADE0-0x8079AE30.  Right edge: the
 *   one-copy `"menu_infomation.cpp"` string (`lbl_805DCCDC`) is referenced only from 0x8030A328 up, so every body
 *   from 0x80308FB4 belongs to `menu/menu_infomation.cpp`; `fn_80308EC0`/`fn_80308F1C` read that unit's
 *   `StatusScreenWork` and call its `fn_8030A1D0`.
 * NAMES. `eft042_set2` is the runtime dump's own name (the family's spawner); `eft_spawn_type_at_area` is a GUESS
 *   (from its body); the map has only `fn_` stems for the rest, so plain definitions are `extern "C"`.
 * RESIDUALS. 3 rows unwritten: 0x80307068-0x80307AE8 (`fn_80307068`, the kind-0 state-0 body over
 *   `jumptable_805DCB68`), 0x80307E44-0x803088FC (`fn_80307E44`, kind-0 state-1), 0x80308A30-0x80308D00
 *   (`fn_80308A30`, kind-2 state-1; it needs the `nw4r::g3d::ScnMdl::CopiedMatAccess` class views).
 *   18 partial rows, including:
 *  - `fn_8030681C`, `fn_803088FC`, `fn_80308F1C`: ours call `get_now_mapno`, `eft004_set`, `rotLocalMatX`,
 *    `rotLocalMatY` and `sysSE_req` by their plain names where retail calls the manglings (`get_now_mapno__Fv`,
 *    `eft004_set__FUcPQ34nw4r4math4VEC3fUlUc`, `rotLocalMatX__FUlPQ34nw4r4math5MTX34`, `sysSE_req__Fl`, ...);
 *    `fn_8030681C` also has a 0x50 frame against retail's 0x60 (retail keeps `f31` live for `lbl_8079ADE8`);
 *  - `fn_8030681C`, `fn_80308D00`, `eft042_set2`: ours fuse `subic.`/`rlwinm.` where retail keeps the compare
 *    separate, and drop the `clrlwi` narrowing of the `u8` arguments (also `fn_80306D6C`, `fn_80308E38`);
 *  - `fn_80306D6C`: retail stores `source_0x30` twice (`NULL`, then the source); ours drops the first store;
 *  - `fn_80306FF0`: ours compares the states 1, 2, 3, 0 where retail compares 0, 1, 2, 3 (case 0 is written last;
 *    docs/ef.md, "The case-0 nested dispatch");
 *  - `fn_80306F10`: retail branches `ble` past the tail call where ours inverts it.
 *   The other 9 partial rows have no recorded cause (`symdiff.py -u ef/fn_8030681C --all`).
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted (the `.sdata2` pool is shared with
 *   `ef/fn_803066F0.c`: a fold candidate); `.text` (0xEFC of 0x2798), extab (0x88 of 0xA0) and extabindex (0xCC of
 *   0xF0) short of the claim and differing; the five plain-named callees above have no map row.
 * SHAPES. `fn_8030681C` calls the two `created` slots through the `EftUvModel` vtable view (slots 7 and 9 of an
 *   `nw4r::ef::Effect`) and reads the joint table as the stage's `Vec` array (`lbl_805DC5E8`).
 *   `fn_80306F10`: `kind` is `u32`, so `kind - 1 <= 1` emits `cmplwi`.
 *   `Eft042Work`'s unions name the per-family tails of the 64-byte `_EFT::work_0x38` block.
 *   `ef/fn_803066F0.c` declares `fn_8030681C`/`fn_80306A94` itself through its private `EftWork` view of `_EFT`.
 */
#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */

#include "nw4r/math.h"
#include "gx.h"

#include "ef.h"
#include "unsplit/ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "sound/fn_800DD1F0.h"
#include "menu/menu_message.h"
#include "pl.h"
#include "fn_8004CAD8.h"
#include "Pl/fn_8028F66C.h"
#include "g3d/g3d_calcworld.h"
#include "enemy/fn_8012BDF4.h"
#include "unsplit/enemy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "menu/menu_infomation.h"

/* The screen work record this range's tail reads (`fn_80308EC0`/`fn_80308F1C`) and the page count
 * they arm it from (`fn_8030A1D0`) live with their owner, `menu/menu_infomation.cpp`'s header: the
 * two screen bodies below the seam are all this unit needs from that unit.  The `EquipListWork`
 * record moved with `equip_list_page_count`, its only reader.
 */

/* One model record of the work area: the pooled model handle the spawner stores and the create
 * result the state-0 bodies file.  Two families disagree about the second word - the kind-2 bodies
 * (fn_80306D6C stores, fn_80307C54 reads) use it as the model's scale factor - so it is a union
 * (rule 5: each member names the site that uses it, and both keep the +0x04 offset).
 * size: 0x08 */
typedef struct Eft042Model {
    MHchar* model;      /* +0x00  `eft_res_model_get`'s pooled model handle */
    union {
        void* created;  /* +0x04  the `res_eft_UV_model_create*` result (effect 41, kind 0) */
        f32   scale;    /* +0x04  the model's uniform scale (kind-2 bodies) */
    } v_0x04;
} Eft042Model;

/* The work area.  `count`/`models` are the head every family shares; the 0x10 bytes at +0x0C are the
 * four `_g3d_work` handles for effect 41 and the kind-0 bodies (which use the last two), a placement
 * offset plus an angle for effect 42's kind-1 body.  size: 0x20 */
typedef struct Eft042Work {
    u32         count;       /* +0x00  the model count `fn_800F8B44`'s record was taken for */
    Eft042Model models[1];   /* +0x04 */
    union {
        /* effect 41 (`ef/fn_803066F0.c`'s `EftWorkData::works`) and the kind-0 bodies, which use
         * only the last two slots (`fn_80306D14`/`fn_80306E04` clear them, `fn_80306F40` releases
         * them, `fn_80307068` fills them). */
        void* works[4];      /* +0x0C */
        struct {             /* size: 0x10 */
            nw4r::math::VEC3 pos;  /* +0x0C  the model's placement offset (kind 1) */
            f32              angle;/* +0x18  its yaw (kind 1) */
        } kind1;
    } v_0x0C;
    void*       resource;    /* +0x1C  the area resource `fn_80306524` stores */
} Eft042Work;

/* The kind-1 work area's own view (the kind-1 types' state bodies): the placement offset and its
 * scale sit in the same 0x10 bytes the effect-41 view calls `works`, the model's material colour is
 * what the type-7 state-2 body walks, and a second scale sits two records further on.
 * size: 0x64 (the extent the kind-1 bodies prove) */
typedef struct Eft042WorkKind1 {
    u32             count;        /* +0x00 */
    MHchar*         model;        /* +0x04 */
    u8              pad_0x08[0x4];/* +0x08 */
    union {
        nw4r::math::VEC3 offset;  /* +0x0C  the placement offset (kind-1 state-0/1 bodies) */
        _GXColor          color;  /* +0x0C  the material colour (the type-7 state-2 body) */
    } v_0x0C;
    f32             field_0x18;   /* +0x18  the offset's own scale */
    u8              pad_0x1C[0x60 - 0x1C]; /* +0x1C */
    f32             field_0x60;   /* +0x60  the second scale the state-1 body compares against */
} Eft042WorkKind1;

/* The created model `res_eft_UV_model_create*` returns (an `nw4r::ef::Effect`): its vtable at +0x00.
 * Only the two slots this range dispatches are named; the rest keep their slot offset and the table
 * itself is another TU's (rule 10).  size: 0x28 */
typedef struct EftUvModelVtbl {
    void (*slot_0x00)(void*);   /* +0x00 */
    void (*slot_0x04)(void*);   /* +0x04 */
    void (*slot_0x08)(void*);   /* +0x08 */
    void (*slot_0x0C)(void*);   /* +0x0C */
    void (*slot_0x10)(void*);   /* +0x10 */
    void (*slot_0x14)(void*);   /* +0x14 */
    void (*slot_0x18)(void*);   /* +0x18 */
    /* the frame the created model is put at when the effect starts its second step
     * (`fn_8030681C` calls it with 0.0f); it takes the frame as an `f32`. */
    void (*setFrame_0x1C)(void* self, f32 frame);  /* +0x1C */
    void (*slot_0x20)(void*);   /* +0x20 */
    /* the retire call `fn_8030681C` makes once the step timer has run out. */
    void (*retire_0x24)(void* self);               /* +0x24 */
} EftUvModelVtbl;

typedef struct EftUvModel {
    EftUvModelVtbl* vtbl;        /* +0x00 */
} EftUvModel;                    /* size: 0x04 - only the vtable pointer is reached here */

/* The `+0x13C` word of the source record as the kind-1 body uses it: a pointer whose four-byte
 * header is followed by the model handle (`fn_80307AE8` reaches `+0x04` of it for `get_joint_wpos`).
 * size: 0x08 */
typedef struct EnemyModelRef {
    u32     pad_0x00;   /* +0x00 */
    MHchar* model;      /* +0x04 */
} EnemyModelRef;

/* ---------------------------------------------------------------------------------------------------
 * the unit's own claimed data (.data 0x805DC5E8-0x805DCC50, .sdata2 0x8079ADE0-0x8079AE30), declared, never
 * defined (playbook 29)
 * ------------------------------------------------------------------------------------------------- */

extern u8 lbl_805DC7E4[];      /* the per-type kind table: 0/1/2 select the state-0 body */
extern Vec lbl_805DC5E8[4];    /* the stage's random placement table (four engine vectors) */
extern f32 lbl_8079ADE4;       /* 0.0f  */
extern f32 lbl_8079ADE8;       /* 20.0f */
extern f32 lbl_8079ADEC;       /* 4.0f  */
extern f32 lbl_8079ADF4;       /* 0.5f  */
extern f32 lbl_8079AE10;       /* -5.0f */
extern f32 lbl_8079AE14;       /* 10.0f */
extern f32 lbl_8079AE18;       /* 0.0f  */
extern f32 lbl_8079AE1C;       /* 2.5f  */
extern f32 lbl_8079AE20;       /* 0.25f */
extern f32 lbl_8079AE24;       /* 1.0f  */
extern u16 lbl_805DC724[];     /* per-type model/animation id (kind 1/2 bodies) */
extern u32 lbl_805DC798[];     /* per-type step-timer length (kind-2 state-0 body) */
extern u32 lbl_805DCB1C[];     /* per-type joint number (the type-7 state-2 body) */

/* ---------------------------------------------------------------------------------------------------
 * this unit's own bodies, forward-declared (address order)
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_8030681C(_EFT* self);
extern "C" void fn_80306A84(_EFT* self);
extern "C" void fn_80306A94(_EFT* self);
extern "C" void eft_spawn_type_at_area(_ENEMY_WORK* source, u8 type);
extern "C" void fn_80306B10(_ENEMY_WORK* source, u8 type, s32 timer);
extern "C" _EFT* fn_80306BFC(u8 type, u8 area);
extern "C" void fn_80306D14(MHchar* model, u8 type);
extern "C" void fn_80306D6C(void* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale,
                            u8 area);
extern "C" _EFT* fn_80306E04(u8 type, u8 area);
extern "C" void fn_80306F10(_EFT* self);
extern "C" void fn_80306F40(_EFT* self);
extern "C" void fn_80306FB4(_EFT* self);
extern "C" void fn_80306FF0(_EFT* self);
extern "C" void fn_80307068(_EFT* self);
extern "C" void fn_80307AE8(_EFT* self);
extern "C" void fn_80307C54(_EFT* self);
extern "C" void fn_80307E08(_EFT* self);
extern "C" void fn_80307E44(_EFT* self);
extern "C" void fn_803088FC(_EFT* self);
extern "C" void fn_80308A30(_EFT* self);
extern "C" void fn_80308D00(_EFT* self);
extern "C" void fn_80308E34(_EFT* self);
extern "C" void fn_80308E38(_EFT* self, u32 visible, u8 from, u8 to);
extern "C" void fn_80308EC0(StatusScreenWork* self);
extern "C" s32  fn_80308F1C(StatusScreenWork* self);

/* Neighbours of this range the map leaves plain; the ones an owner unit already has a header for are
 * declared there and included above (rule 2), the rest sit here until their owner writes one. */
extern "C" void eft004_set(u8 id, nw4r::math::VEC3* pos, f32 scale, u32 a, u8 area);
extern "C" void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
extern "C" void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
extern "C" void sysSE_req(s32 id);

/* `eft_res_slot_get`'s working area (64 bytes per record) and the model pool are the resource manager's;
 * the two entry points here are the ones this unit pools from. */

/* ---------------------------------------------------------------------------------------------------
 * 0x8030681C - the effect-41 machine's state-1 step
 * ------------------------------------------------------------------------------------------------- */

/* Walks the work area's model records: places each model when its step timer expires, reveals its joints, walks it
 * and retires it after the second timer, then hands its position to `eft004_set`. */
extern "C" void fn_8030681C(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    u8 mapno = get_now_mapno();
    nw4r::math::VEC3 pos;
    Vec* pos_tbl;
    u32 pos_num;
    u32 i;

    VEC3_ctor(&pos);
    pos_tbl = NULL;
    if ((mapno == 2 || mapno == 13) && self->area_0x44 == 10) {
        pos_tbl = lbl_805DC5E8;
        pos_num = 4;
    }

    for (i = 0; i < work->count; i++) {
        Eft042Model* model = &work->models[i];

        switch (self->field_0x06) {
        case 0:
            if (--self->timer_0x0C < 0) {
                u32 idx;
                u32 joint;
                u32 joints;

                self->field_0x06 = 1;

                ((EftUvModel*)model->v_0x04.created)->vtbl->setFrame_0x1C(
                    model->v_0x04.created, lbl_8079ADE4);

                idx = ran_suu(1) % pos_num;
                vec_to_mh_vec3(&model->model->pos_0x04, &pos_tbl[idx]);

                joints = model->model->get_joint_num();
                for (joint = 0; joint < joints; joint++) {
                    if (joint == ((u8*)work->resource)[idx]) {
                        model->model->setVisibility(joint, true);
                    } else {
                        model->model->setVisibility(joint, false);
                    }
                }
            }
            break;
        case 1:
            if (++self->timer_0x0C > 200) {
                self->field_0x06 = 0;
                self->timer_0x0C = ran_suu(1) & 0x1F;
            } else {
                model->model->move(0);
                ((EftUvModel*)model->v_0x04.created)->vtbl->retire_0x24(model->v_0x04.created);
                eft_res_models_spawn(self, (void**)&work->models[i], 2, 1, NULL);
            }
            break;
        }

        switch (200 - self->timer_0x0C) {
        case 15:
        case 20:
        case 25:
        case 30:
        case 35:
            fn_800DD514(&model->model->pos_0x04);
            break;
        }

        copyVec3(&pos, &model->model->pos_0x04);
        pos.y -= lbl_8079ADE8;
        eft004_set(0, &pos, lbl_8079ADEC, 0, self->area_0x44);
    }
}

/* The machine's state-2 step: one step further into the effect. */
extern "C" void fn_80306A84(_EFT* self)
{
    self->state_0x05++;
}

/* The machine's state-3 step (and every failure path): retire the effect. */
extern "C" void fn_80306A94(_EFT* self)
{
    eft_res_slot_release(self);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80306A98, 0x80306B10, 0x80306B60 - the effect-42 spawn entries
 * ------------------------------------------------------------------------------------------------- */

/* Spawns effect 42 for `source` (type 3 becomes type 4 when the source's +0x228 bits 1-2 are set) and files it. */
extern "C" void eft_spawn_type_at_area(_ENEMY_WORK* source, u8 type)
{
    _EFT* eft = fn_80306BFC(type, source->area_no);

    if (eft != NULL) {
        if (type == 3 && (source->field_0x228 & 6) != 0) {
            eft->type_0x02 = 4;
        }
        eft->source_0x30 = source;
        eft->timer_0x0C = 0;
    }
}

/* The same spawn, but with the step timer the caller picks (the state-0 body's own timer). */
extern "C" void fn_80306B10(_ENEMY_WORK* source, u8 type, s32 timer)
{
    _EFT* eft = fn_80306BFC(type, source->area_no);

    if (eft != NULL) {
        eft->source_0x30 = source;
        eft->timer_0x0C = timer;
    }
}

/* Takes a work block from the effect pool, fills its model handle and places the effect from the source record. */
void eft042_set2(_ENEMY_WORK* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot)
{
    _EFT* eft = fn_80306BFC(type, source->area_no);

    if (eft != NULL) {
        if (type == 3 && (source->field_0x228 & 6) != 0) {
            eft->type_0x02 = 4;
        }
        copyVec3(&eft->pos_0x18, pos);
        eft_rot_vec_copy(&eft->rot_0x24, rot);
        eft->source_0x30 = source;
        eft->timer_0x0C = 0;
    }
}

/* Pools an effect-42 record and its work area, checks the model handle and stamps the record and its hooks;
 * returns NULL outside the current area or when a pool is exhausted. */
extern "C" _EFT* fn_80306BFC(u8 type, u8 area)
{
    _EFT* eft;
    Eft042Work* work;
    MHchar** handles;
    u32 i;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)eft_res_slot_get(28);
    if (eft == NULL) {
        return NULL;
    }

    eft->type_0x02 = type;
    eft->release_0x40 = fn_80306F10;
    eft->dispatch_0x34 = fn_80306FF0;

    work = (Eft042Work*)eft->work_0x38;
    work->count = 1;
    memset(&work->v_0x0C.works[2], 0, 8);

    handles = &work->models[0].model;
    for (i = 0; i < work->count; i++) {
        handles[i] = (MHchar*)eft_res_model_get();
        if (handles[i] == NULL) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 42;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft->field_0x10 = 0;
    eft_state_flags_set(eft, 0, 0);

    return eft;
}

/* Spawns effect 42 from a model: the record is placed at the model's rotation and the model is filed
 * as the effect's origin. */
extern "C" void fn_80306D14(MHchar* model, u8 type)
{
    _EFT* eft = fn_80306E04(type, model->area_0x16);

    if (eft != NULL) {
        eft_rot_vec_copy(&eft->rot_0x24, &model->rot_0x54);
        eft->source_0x30 = model;
    }
}

/* The same spawn, placed by the caller: the position, the rotation, the model scale kept in the work
 * area's second word, and the origin record. */
extern "C" void fn_80306D6C(void* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale,
                            u8 area)
{
    _EFT* eft = fn_80306E04(type, area);

    if (eft != NULL) {
        copyVec3(&eft->pos_0x18, pos);
        eft_rot_vec_copy(&eft->rot_0x24, rot);
        eft->source_0x30 = NULL;
        eft->source_0x30 = source;
        ((Eft042Work*)eft->work_0x38)->models[0].v_0x04.scale = scale;
    }
}

/* Pools an effect-42 record like `fn_80306BFC`, stamping the requested type and the `eft_state_flags_set` mask 4. */
extern "C" _EFT* fn_80306E04(u8 type, u8 area)
{
    _EFT* eft;
    Eft042Work* work;
    MHchar** handles;
    u32 i;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)eft_res_slot_get(28);
    if (eft == NULL) {
        return NULL;
    }

    work = (Eft042Work*)eft->work_0x38;
    work->count = 1;

    handles = &work->models[0].model;
    for (i = 0; i < work->count; i++) {
        handles[i] = (MHchar*)eft_res_model_get();
        if (handles[i] == NULL) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 42;
    eft->type_0x02 = type;
    eft->field_0x04 = 0;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft->timer_0x0C = 0;
    eft_state_flags_set(eft, 0, 4);

    eft->release_0x40 = fn_80306F10;
    eft->dispatch_0x34 = fn_80306FF0;

    return eft;
}

/* ---------------------------------------------------------------------------------------------------
 * the work area's release paths (the record's `release_0x40` hook and its two arms)
 * ------------------------------------------------------------------------------------------------- */

/* Destroys the record by kind: kinds 1 and 2 release the model handles, kind 0 also the `_g3d_work` handles. */
extern "C" void fn_80306F10(_EFT* self)
{
    u32 kind = lbl_805DC7E4[self->type_0x02];

    if (kind - 1 <= 1) {
        fn_80306FB4(self);
    } else if (kind == 0) {
        fn_80306F40(self);
    }
}

/* The kind-0 release: give the two `_g3d_work` handles the state-0 body took back, then release the
 * model handles and empty the work area. */
extern "C" void fn_80306F40(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    s32 i;

    for (i = 1; i >= 0; i--) {
        void* wk = work->v_0x0C.works[i + 2];
        if (wk != NULL) {
            push_g3d_wk((struct _g3d_work*)wk);
        }
    }
    fn_800F8A44(&work->models[0].model, work->count);
    work->count = 0;
}

/* The kinds-1/2 release: the model handles only. */
extern "C" void fn_80306FB4(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;

    fn_800F8A44(&work->models[0].model, work->count);
    work->count = 0;
}

/* The record's update hook: the outer machine is the state byte (`_EFT::state_0x05`), and its first
 * state is the per-type machine the kind table selects. */
extern "C" void fn_80306FF0(_EFT* self)
{
    switch (self->state_0x05) {
    case 1:
        fn_80307E08(self);
        break;
    case 2:
        fn_80308D00(self);
        break;
    case 3:
        fn_80308E34(self);
        break;
    case 0:
        switch (lbl_805DC7E4[self->type_0x02]) {
        case 0:
            fn_80307068(self);
            break;
        case 1:
            fn_80307AE8(self);
            break;
        case 2:
            fn_80307C54(self);
            break;
        }
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80307E08..0x80308A30 - the second state's per-kind bodies
 * ------------------------------------------------------------------------------------------------- */

/* The state-1 dispatch: the same kind table as the state-0 machine, one body per kind. */
extern "C" void fn_80307E08(_EFT* self)
{
    switch (lbl_805DC7E4[self->type_0x02]) {
    case 0:
        fn_80307E44(self);
        break;
    case 1:
        fn_803088FC(self);
        break;
    case 2:
        fn_80308A30(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80308E34..0x80308F1C - the machine's retire arm and the screen tail
 * --------------------------------------------------------------------------------------------------- */

/* The state-3 arm: retire the effect record. */
extern "C" void fn_80308E34(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Makes exactly one joint of the work area's model visible and hides every joint in `[from, to]` -
 * the joint-walk the state-1 body and `eft035.cpp`'s `fn_802F3954` share. */
extern "C" void fn_80308E38(_EFT* self, u32 visible, u8 from, u8 to)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    u32 joint;

    for (joint = from; joint <= to; joint++) {
        if (joint == visible) {
            work->models[0].model->setVisibility(joint, true);
        } else {
            work->models[0].model->setVisibility(joint, false);
        }
    }
}

/* Arms the screen: the mode byte, the cursor reset, the equipment page the `+0x190` record hands out
 * (the returned count picks the last-page byte) and the page index. */
extern "C" void fn_80308EC0(StatusScreenWork* self)
{
    self->field_0x14 = 7;
    self->field_0x23A = 0;
    self->field_0x1AF = (s8)(fn_8030A1D0(self->equip, 0) + 2);
    self->field_0x1AE = 0;
}

/* Folds the screen's flag words into one mask: the SE bit plays the confirm SE (returns 2), the scroll bits step the
 * page through `menu_cursor_step` (returns 0). */
extern "C" s32 fn_80308F1C(StatusScreenWork* self)
{
    s32 result = 0;
    u16 flags = (u16)(self->field_0x04 | (self->field_0x08 & 0xF));

    if ((flags & 0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if ((flags & 0xC) != 0) {
        self->field_0x1AE = (s8)menu_cursor_step(self->field_0x1AE, self->field_0x1AF, flags, 4, 8);
    }
    return result;
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80307AE8..0x80308D00 - the per-kind state bodies
 * --------------------------------------------------------------------------------------------------- */

/* Kind 1, state 0: bind the type's animation to the pooled model, place the effect on the source
 * model's joint 7, and give the placement offset its angle, scale and start size. */
extern "C" void fn_80307AE8(_EFT* self)
{
    Eft042WorkKind1* work = (Eft042WorkKind1*)self->work_0x38;
    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    self->state_0x05++;

    if (res_eft_model_create(work->model, lbl_805DC724[self->type_0x02], 0) == NULL) {
        fn_80308E34(self);
        return;
    }
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }

    ((EnemyModelRef*)source->field_0x13C)->model->get_joint_wpos(7, &self->pos_0x18);

    setVector3(&offset, lbl_8079AE10, lbl_8079AE14, lbl_8079AE18);
    rotVecY(&offset, self->rot_0x24.y);
    copyVec3(&work->v_0x0C.offset, &offset);
    work->field_0x18 = lbl_8079AE1C;

    if (eft_water_state_ck(self) == 1) {
        vec3_scale_in_place(&work->v_0x0C.offset, lbl_8079ADF4);
        work->field_0x18 *= lbl_8079AE20;
    }

    setVector3(&work->model->scale_0x1C, lbl_8079AE24, lbl_8079AE24, lbl_8079AE24);
    self->timer_0x0C = 40;
    work->model->field_0x28 = 0;
    work->model->field_0x2C = self->rot_0x24.y;
    work->model->field_0x30 = 0;
    fn_80307E08(self);
}

/* Binds the type's animation to the pooled model, places, spins and scales it, and sets the colour types' blend
 * and TEV colours (kind 2, state 0). */
extern "C" void fn_80307C54(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    MHchar* model = work->models[0].model;
    _GXColor color;
    u32 i;

    self->state_0x05++;

    if (res_eft_model_create(model, lbl_805DC724[self->type_0x02], 340) == NULL) {
        fn_80308E34(self);
        return;
    }

    copyVec3(&model->pos_0x04, &self->pos_0x18);
    eft_rot_vec_copy((_CP_VECTOR*)&model->field_0x28, &self->rot_0x24);
    model->field_0x2C += 16384;
    model->field_0x28 += ran_suu(0);
    model->setVisibility(1, false);
    setVector3(&model->scale_0x1C, work->models[0].v_0x04.scale, work->models[0].v_0x04.scale,
               work->models[0].v_0x04.scale);

    self->flag_0x01 = 1;
    self->timer_0x0C = lbl_805DC798[self->type_0x02];

    switch (self->type_0x02) {
    case 11:
        color.r = 102;
        color.g = 238;
        color.b = 255;
        color.a = 255;
        break;
    case 12:
        color.r = 255;
        color.g = 0;
        color.b = 0;
        color.a = 255;
        for (i = 0; i < 2; i++) {
            model->setMatAlphaBlendMode(i, GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
                                        GX_LO_CLEAR);
        }
        break;
    }

    for (i = 0; i < 2; i++) {
        model->setTevKColor(i, GX_KCOLOR3, &color);
    }

    fn_80307E08(self);
}

/* Kind 1, state 1: walk the placement offset down to the source model's joint every frame, rotate
 * its matrix by the effect's two angles and hand it to the model. */
extern "C" void fn_803088FC(_EFT* self)
{
    Eft042WorkKind1* work = (Eft042WorkKind1*)self->work_0x38;
    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);

    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
    } else if (self->pos_0x18.z < work->field_0x60 || --self->timer_0x0C < 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
    } else {
        work->v_0x0C.offset.y -= work->field_0x18;
        addVec3To(&self->pos_0x18, &work->v_0x0C.offset);
        self->rot_0x24.x -= 2185;
        mtx34_identity(&mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->model->move2(&mtx, 0);
        eft_res_models_spawn(self, (void**)&work->model, 2, work->count, NULL);
    }
}

/* The type-7 state-2 body: follow the source model's joint, set the material's colour from the work
 * area's alpha (dropped by the effect's own +0x08 byte every frame) and walk the model along. */
extern "C" void fn_80308D00(_EFT* self)
{
    Eft042WorkKind1* work = (Eft042WorkKind1*)self->work_0x38;
    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;

    if (self->type_0x02 == 7) {
        if (em_work_die_ck(source)) {
            self->state_0x05++;
            return;
        }
        if (--self->timer_0x0C <= 0) {
            self->state_0x05++;
            return;
        }

        work->v_0x0C.color.a -= self->demo_flag_0x08;
        get_joint_wpos_em(source, lbl_805DCB1C[self->type_0x02], &self->pos_0x18);
        copyVec3(&work->model->pos_0x04, &self->pos_0x18);
        eft_rot_vec_copy(&work->model->rot_0x54, (_CP_VECTOR*)&source->field_0x1BC);
        work->model->setMatColor(0, GX_COLOR0A0, work->v_0x0C.color, false);
        work->model->move(0);
        eft_res_models_spawn(self, (void**)&work->model, 2, work->count, NULL);
    } else {
        self->state_0x05++;
    }
}
