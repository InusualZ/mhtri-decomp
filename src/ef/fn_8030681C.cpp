/* ef/fn_8030681C.cpp - the `eft041`/`eft042` effect family's machine, `.text` 0x8030681C..0x80308FB4
 * (26 functions, 10136 B), extab 0x80015AB4..0x80015B54 (20 records) and extabindex
 * 0x80034674..0x80034764 (20 x 12 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: the runtime dump names only `eft042_set2`, and
 * every other address in the range is the dump's placeholder `zz_XXXXXXXX_`; a real name would need
 * the map and the source in one edit).
 *
 * Registration (docs/plan.md 12).  Class 4 decided the name and class 2 the module.  The range is the
 * head of the maximal unclaimed run between the two registered units `ef/fn_803066F0.c` (ends
 * 0x8030681C) and `hud/fn_80324F7C.c` (starts 0x80324F7C).  The module is `ef`: the left bracket is an
 * `ef` unit, the range's first functions are the eft041/042 effect machine (its first body drives the
 * `_EFT` record's work area, and the range defines `eft042_set2`), and the naming scheme of the `ef`
 * siblings is `eft0XX.cpp`.  Language C++ (the range defines seven genuinely mangled symbols;
 * `langcheck.py --unit` on the range reports `c++ high` from
 * `eft042_set2__FP11_ENEMY_WORKUcPQ34nw4r4math4VEC3P10_CP_VECTOR`), so every definition whose map
 * name is plain is `extern "C"` so its emitted name stays the map's stem and objdiff can pair it
 * (playbook row 42).
 *
 * SEAM (settled 2026-09-26, `.pi/notes/seam-round.md`).  The right edge is 0x80308FB4, not the
 * 0x8030D338 this unit was first registered with.  `lbl_805DCCDC` is the bare source name
 * `"menu_infomation.cpp"`, it has **exactly one copy in the whole DOL**, and the functions whose
 * relocations name it span 0x8030A328 .. `Set_equip_column_arrangement` (0x8031A244) - a TU-local
 * static has one emitter, so that whole run is one TU.  Cutting it at 0x8030D338 leaves 2 referrers
 * above and 9 below (a false boundary); at 0x80308FB4 it leaves 0 and 11, which is consistent, so
 * every body from 0x80308FB4 up belongs to `menu/menu_infomation.cpp`, whose range now starts exactly
 * there.  One constant stays: the two screen bodies below the cut, `fn_80308EC0` and `fn_80308F1C`,
 * are this unit's because they sit below 0x80308FB4; they read the shared `StatusScreenWork` record
 * from `include/menu/menu_infomation.h` and call that unit's `fn_8030A1D0`.  The effect machine's own
 * edge is where its two clusters part:
 *   0x8030681C..0x80308E34 (22 rows, 19 drawn)  the eft041/eft042 effect machine: `_EFT` work areas
 *       at +0x38, `res_eft_*` model creation, `MHchar` transforms, `eftGetKey*` colour keys, the
 *       `_ENEMY_WORK` source at +0x30.  `eft042_set2` (0x80306B60) is the family's spawner.
 *   0x80308E34..0x80308FB4 (4 rows, all drawn)  the machine's retire arm (`fn_80308E34`) and the
 *       screen's entry: `fn_80308E38` (the joint walk `eft035.cpp` shares), `fn_80308EC0` and
 *       `fn_80308F1C`.
 * The section split follows the same boundary: `equip_info_update` (0x80308FB4) is the first function after it and the
 * first `extabindex` record of the next unit, so 20 records stay here and 20 is exactly the count
 * this range's framed functions need.
 *
 * Types.  `_EFT` (include/ef.h) is the 0x48-byte record `fn_800F8788` pools; its `work_0x38` is the
 * 64-byte per-effect work area `fn_800F8B44` hands out.  The work area's head is the same everywhere
 * (a model count, the pooled `MHchar` handle, the `res_eft_*_model_create` result), but its tail is
 * per family: effect 41 (and effect 42's kind-0 bodies) read four `_g3d_work` handles at +0x0C, effect
 * 42's kind-1 body a `VEC3` and an `f32` there, and kind-2 bodies an `f32` scale in the create-result
 * word - so the two sites that disagree are spelled as unions on the one `Eft042Work` here, each union
 * member naming the site that uses it (rule 5).  `MHchar` is include/pl.h's record (its `area_0x16`
 * and `rot_0x54` are what the spawners copy); `res_eft_UV_model_create*` returns the `nw4r::ef::Effect`
 * whose two dispatched vtable slots are the local `EftUvModel` view.
 *
 * Data.  The range's `.data` jump tables (`jumptable_805DCB68`, `jumptable_805DCBB4`,
 * `jumptable_805DCC00`, `jumptable_805DCD08`), the per-type kind table `lbl_805DC7E4`, the type/anim
 * tables and the `.sdata2` float pool are `extern`-declared by their map names and never defined
 * (playbook 29).
 *
 * State.  23 of the range's 26 functions are written and every written row is at or above the 80 %
 * bar (5 byte-identical); the official unit metric is **35.491714 % fuzzy, 144 / 10136 `.text`
 * bytes** (`build/RMHE08/report.json`):
 *
 *   100.00  fn_80306A84  fn_80306A94  fn_80306FB4  fn_80307E08  fn_80308E34
 *    98.68  fn_80307AE8      97.27  fn_80306D14      96.50  fn_80306B10      95.65  fn_80308EC0
 *    93.99  fn_80306BFC      93.72  fn_80306E04      93.20  fn_80306FF0      93.07  fn_80306F40
 *    89.17  fn_80306A98      88.86  fn_8030681C      87.93  fn_80307C54      86.41  eft042_set2
 *    86.27  fn_80308D00      85.29  fn_80308E38      84.96  fn_803088FC      84.87  fn_80308F1C
 *    82.50  fn_80306F10      82.11  fn_80306D6C
 *
 * The four bodies the range's head needed beyond the obvious shape, with what settled them:
 *   - `fn_8030681C` 88.86: the two `created` slots are called through a `EftUvModel` vtable view
 *     (slots 7 and 9 of an `nw4r::ef::Effect`), and the joint/position table is the stage's `Vec`
 *     array (`lbl_805DC5E8`), not a `VEC3` array - `vec_to_mh_vec3` converts it into the model's
 *     `VEC3` position.
 *   - `fn_80306FF0` 93.20: the outer switch's case 0 must be written **last**; MWCC then emits the
 *     compare chain ascending (0,1,2,3) with case 0's nested-switch body after the whole switch,
 *     which is the target's layout.  Case 0 first scores 72.83.
 *   - `fn_80306F10` 82.50: `kind` must be `u32`, so `kind - 1 <= 1` emits `cmplwi` (the target's
 *     form); an `int` emits `cmpwi` (77.50) and a `switch` on the kind (64.58).
 *   - `fn_80306D6C` 82.11: the work area's `+0x08` word is stored twice through the effect record's
 *     origin (`source_0x30 = NULL; source_0x30 = source;`); collapsing the two stores loses 8 points.
 *
 * Residuals (what each still needs):
 *   fn_80307068 (0xA80)  the kind-0 state-0 body: a 19-way type jump table (`jumptable_805DCB68`)
 *                        whose arms build one to three pooled models from `lbl_805DC724`'s per-type
 *                        animation ids and drive their key frames.  It is the range's largest single
 *                        body, and the arms are what the `.data` run 0x805DC5E8..0x805DCD08 (the four
 *                        jump tables and 27 labels) is emitted for, so it has to be measured together
 *                        with the data claim (5d).
 *   fn_80307E44 (0xAB8)  the kind-0 state-1 body (the same family's per-frame walk, `eftGetKey*`).
 *   fn_80308A30 (0x2D0)  the kind-2 state-1 body: three `nw4r::g3d::ScnMdl::CopiedMatAccess` round
 *                        trips over `ResTexSrt`/`GetEffectMtx`/`SetEffectMtx`; it needs the full nw4r
 *                        g3d class views (include/nw4r/g3d/scnmdl.h carries only part of them today).
 * Those three are the whole remainder: the range's other 23 rows are written.
 *
 * Data (measured, 5d).  Our object emits **no** `.data`/`.sdata`/`.sdata2`/`.rodata` at all: `objdump
 * -h` on `build/RMHE08/src/ef/fn_8030681C.o` against the split target object
 * (`build/RMHE08/obj/ef/fn_8030681C.o`) differs only in `.text`, `extab` and `extabindex` - all three
 * because four bodies are still unwritten - and no data section is `ours-extra`.  Nothing beyond the
 * three claimed ranges is registered: the range's own `.data` run (the four `jumptable_805D*` switch
 * tables and the 27 labels from 0x805DC5E8 to 0x805DCD08) and its `.sdata2` float pool stay unclaimed
 * until the giant bodies that emit them are written, because a claim our object does not emit is a
 * false claim (5d, playbook 29/58).
 *
 * Rule-2 note.  `ef/fn_803066F0.c` (the range's left neighbour) declares `fn_8030681C` and
 * `fn_80306A94` itself, which the lint resolves to this unit.  They cannot move to a header here: the
 * neighbour's `EftWork` is its own private view of the same record this file spells `_EFT`, and a
 * shared header would have to pick one of the two names.  Recorded as a residual for the header
 * consolidation pass (the lint's own "unsplit gap" reasoning).
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
#include "sound/fn_800DD1F0.h"
#include "menu/fn_802A6624.h"
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
 * record moved with `fn_8030B790`, its only reader.
 */

/* One model record of the work area: the pooled model handle the spawner stores and the create
 * result the state-0 bodies file.  Two families disagree about the second word - the kind-2 bodies
 * (fn_80306D6C stores, fn_80307C54 reads) use it as the model's scale factor - so it is a union
 * (rule 5: each member names the site that uses it, and both keep the +0x04 offset).
 * size: 0x08 */
typedef struct Eft042Model {
    MHchar* model;      /* +0x00  `fn_800F8914`'s pooled model handle */
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
 * data this unit references but does not own (playbook 29: declared, never defined)
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
extern "C" void fn_80306A98(_ENEMY_WORK* source, u8 type);
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
extern "C" u8 get_now_mapno(void);
extern "C" void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
extern "C" void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
extern "C" void sysSE_req(s32 id);

/* `fn_800F8788`'s working area (64 bytes per record) and the model pool are the resource manager's;
 * the two entry points here are the ones this unit pools from. */

/* ---------------------------------------------------------------------------------------------------
 * 0x8030681C - the effect-41 machine's state-1 step
 * ------------------------------------------------------------------------------------------------- */

/* Walks the work area's model records.  While the step timer runs down, each model's substate is 0;
 * the step the timer expires, the model is placed on the random position the stage's table picked,
 * its joints are made visible one at a time and the state advances to the `field_0x06 == 1` arm,
 * which walks the model along and retires the created object when the second timer (a 0..31 draw from
 * `ran_suu`) has run out.  Either way the model's position is copied into the effect's placement
 * buffer, dropped by 20 units and handed to `eft004_set` with the area's parameter. */
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
                fn_800F93D8(self, (void**)&work->models[i], 2, 1, NULL);
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
    fn_800F886C(self);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80306A98, 0x80306B10, 0x80306B60 - the effect-42 spawn entries
 * ------------------------------------------------------------------------------------------------- */

/* Spawns effect 42 for `source` and files it: type 3 from a source whose +0x228 flags word has either
 * of bits 1-2 set becomes the type-4 model, the source is stored as the effect's origin and the step
 * timer starts at 0. */
extern "C" void fn_80306A98(_ENEMY_WORK* source, u8 type)
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

/* The `eft042` family's spawner (the map's own name, a C++ free function - rule 9).  Takes a work
 * block from the effect pool, fills its model handle from the model pool, and places the effect: the
 * position, the rotation and the source record. */
void eft042_set2(_ENEMY_WORK* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot)
{
    _EFT* eft = fn_80306BFC(type, source->area_no);

    if (eft != NULL) {
        if (type == 3 && (source->field_0x228 & 6) != 0) {
            eft->type_0x02 = 4;
        }
        copyVec3(&eft->pos_0x18, pos);
        fn_800FC0D4(&eft->rot_0x24, rot);
        eft->source_0x30 = source;
        eft->timer_0x0C = 0;
    }
}

/* The effect-42 creator: pools the effect record and its work area, takes the one model handle the
 * work area was taken for, checks it, and stamps the record - id 42, the caller's type and area, the
 * `field_0x04` flag word and the two hooks that travel with the record.  Returns NULL when the area is
 * not the current one or either pool is exhausted (and gives the record back in that case). */
extern "C" _EFT* fn_80306BFC(u8 type, u8 area)
{
    _EFT* eft;
    Eft042Work* work;
    MHchar** handles;
    u32 i;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)fn_800F8788(28);
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
        handles[i] = (MHchar*)fn_800F8914();
        if (handles[i] == NULL) {
            fn_800F886C(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 42;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft->field_0x10 = 0;
    fn_800F9DF4(eft, 0, 0);

    return eft;
}

/* Spawns effect 42 from a model: the record is placed at the model's rotation and the model is filed
 * as the effect's origin. */
extern "C" void fn_80306D14(MHchar* model, u8 type)
{
    _EFT* eft = fn_80306E04(type, model->area_0x16);

    if (eft != NULL) {
        fn_800FC0D4(&eft->rot_0x24, &model->rot_0x54);
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
        fn_800FC0D4(&eft->rot_0x24, rot);
        eft->source_0x30 = NULL;
        eft->source_0x30 = source;
        ((Eft042Work*)eft->work_0x38)->models[0].v_0x04.scale = scale;
    }
}

/* The second effect-42 creator: the same pooling as `fn_80306BFC`, but the record is stamped with the
 * type it was asked for (not the work area's), its `field_0x04` word is the `fn_800F9DF4` mask 4, and
 * the two hooks are installed after that call. */
extern "C" _EFT* fn_80306E04(u8 type, u8 area)
{
    _EFT* eft;
    Eft042Work* work;
    MHchar** handles;
    u32 i;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)fn_800F8788(28);
    if (eft == NULL) {
        return NULL;
    }

    work = (Eft042Work*)eft->work_0x38;
    work->count = 1;

    handles = &work->models[0].model;
    for (i = 0; i < work->count; i++) {
        handles[i] = (MHchar*)fn_800F8914();
        if (handles[i] == NULL) {
            fn_800F886C(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 42;
    eft->type_0x02 = type;
    eft->field_0x04 = 0;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft->timer_0x0C = 0;
    fn_800F9DF4(eft, 0, 4);

    eft->release_0x40 = fn_80306F10;
    eft->dispatch_0x34 = fn_80306FF0;

    return eft;
}

/* ---------------------------------------------------------------------------------------------------
 * the work area's release paths (the record's `release_0x40` hook and its two arms)
 * ------------------------------------------------------------------------------------------------- */

/* The record's destroy hook, dispatched on the type's kind: the kinds that keep their placement data
 * in the work area's tail (1 and 2) release the model handles only, kind 0 releases the `_g3d_work`
 * handles too. */
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
    fn_800F886C(self);
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

/* The screen's per-frame SE/scroll step: the `+0x04`/`+0x08` flag words are folded into one 16-bit
 * mask - the SE request bit plays the confirm SE and reports 2, the scroll bits advance the page
 * index through `menu_cursor_step` and report 0. */
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
    if (fn_800F92F4(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }

    ((EnemyModelRef*)source->field_0x13C)->model->get_joint_wpos(7, &self->pos_0x18);

    setVector3(&offset, lbl_8079AE10, lbl_8079AE14, lbl_8079AE18);
    rotVecY(&offset, self->rot_0x24.y);
    copyVec3(&work->v_0x0C.offset, &offset);
    work->field_0x18 = lbl_8079AE1C;

    if (fn_800F9D80(self) == 1) {
        fn_800513F0(&work->v_0x0C.offset, lbl_8079ADF4);
        work->field_0x18 *= lbl_8079AE20;
    }

    setVector3(&work->model->scale_0x1C, lbl_8079AE24, lbl_8079AE24, lbl_8079AE24);
    self->timer_0x0C = 40;
    work->model->field_0x28 = 0;
    work->model->field_0x2C = self->rot_0x24.y;
    work->model->field_0x30 = 0;
    fn_80307E08(self);
}

/* Kind 2, state 0: bind the type's animation to the pooled model, copy the effect's placement onto
 * the model, spin the model's rotation run, set the scale from the work area's scale word, and give
 * the two colour types their material blend and TEV colours. */
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
    fn_800FC0D4((_CP_VECTOR*)&model->field_0x28, &self->rot_0x24);
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

    if (fn_800F92F4(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
    } else if (self->pos_0x18.z < work->field_0x60 || --self->timer_0x0C < 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
    } else {
        work->v_0x0C.offset.y -= work->field_0x18;
        fn_80073F68(&self->pos_0x18, &work->v_0x0C.offset);
        self->rot_0x24.x -= 2185;
        fn_800504D4(&mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        fn_800FBB90(&mtx, &self->pos_0x18);
        work->model->move2(&mtx, 0);
        fn_800F93D8(self, (void**)&work->model, 2, work->count, NULL);
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
        fn_800FC0D4(&work->model->rot_0x54, (_CP_VECTOR*)&source->field_0x1BC);
        work->model->setMatColor(0, GX_COLOR0A0, work->v_0x0C.color, false);
        work->model->move(0);
        fn_800F93D8(self, (void**)&work->model, 2, work->count, NULL);
    } else {
        self->state_0x05++;
    }
}
