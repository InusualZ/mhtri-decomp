/* ef/fn_8010BDE4.cpp - the player/enemy action-effect frame handlers, `.text` 0x8010BDE4..0x8010D1A8.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: every address answers `zz_XXXXXXXX_`, which is not
 * a name; the file's own symbols are bare `.text` entries in config/RMHE08/symbols.txt).
 *
 * Registration evidence (brief section 2, in order):
 *   1. no `__FILE__`/assert source-name string exists anywhere in the range (the 16 `.text` blocks of
 *      the old auto split reference only data labels - `lbl_8059F518`, `lbl_8059F574`, `lbl_80791820`,
 *      `lbl_807967E8`..`lbl_80796834` and `pRoot` - none of which is a `.c`/`.cpp` name);
 *   2. `dumpmap.py lookup` gives only `zz_XXXXXXXX_` for every function, which is not evidence;
 *   3. module = `ef`, class 3: the range sits in the `ef` link band directly below
 *      `ef/fn_8010D1A8.c` (0x8010D1A8..0x801121DC) and above the `ef/fn_80104BD0.c` /
 *      `ef/eft009.cpp` cluster, and it drives the same `_EFT` pool records those units own
 *      (`fn_800F8788`/`fn_800F8914`/`fn_800F8A44`/`fn_800F886C`, `fn_800F9D80`/`fn_800F9DF4`,
 *      `res_eft_model_create`, the `MHchar` members) - so it goes in the `ef` lib block of
 *      configure.py, next to its neighbours;
 *   4. name = the map's own stem `fn_8010BDE4` (class 4): no evidence supports a better one and the
 *      `ef` siblings use exactly this scheme (`ef/fn_80104BD0.c`, `ef/fn_8010D1A8.c`).
 *
 * Language: C++.  The map carries no conclusive C++ evidence for the range's own symbols (they are
 * plain `fn_XXXXXXXX`, so rule 7's deferral applies), but every callee it reaches is a C++ mangling -
 * a `MHchar` member (`move`, `move2`), a namespaced nw4r function and the `_PLW`/`_ENEMY_WORK`
 * accessors - and section 6.5 rule 9 (checked, not deferred) forbids spelling those manglings as
 * callable identifiers.  A C translation unit cannot reach them any other way, so the range is built
 * as C++ with its own symbols declared `extern "C"` (exactly the shape `sound/fn_800EF7D8.cpp` and
 * `ef/effect.cpp` landed with); the language hint in the brief was C++ (medium) for the same reason.
 *
 * What the unit is.  Two effect families share the `_EFT` record (`ef.h`), reached through
 * `_EFT::work_0x38`:
 *   * the light family - `fn_8010C468` allocates the effect from the 0x50-entry pool, installs the
 *     `fn_8010C77C` state dispatcher and the `fn_8010C710` release hook, and pools `count` light
 *     handles; `fn_8010C554`/`fn_8010C5DC`/`fn_8010C680` are the spawn wrappers (a 5-light player
 *     effect, a 1-light parameterised one, a 5-light one with a y offset), and `fn_8010C7B8` /
 *     `fn_8010C8F8` are its first two per-frame states.
 *   * the frame family - `fn_8010BDE4` seeds two 0x48-byte per-model records (random timers, angles
 *     and per-frame steps) and hands off to `fn_8010C0E0`, which places each model along its angles
 *     for the state the record is in.
 * `fn_8010CD68`, `fn_8010CE80` and `fn_8010CFCC`/`fn_8010D12C` are the light/matrix helpers the
 * second half drives.
 *
 * The work block layouts are the two per-family views of `_EFT::work_0x38`; both are reconstructed
 * from the field offsets in `.text` (no DWARF in an MWCC object) and carry their size.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit ef/fn_8010BDE4.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"

/* ---------------------------------------------------------------------------------------------------
 * the two per-family `_EFT::work_0x38` blocks
 * ------------------------------------------------------------------------------------------------- */

/* The light family's block, allocated by `fn_800F8788(0x50)`: the pooled light handles the state
 * machine walks, the effect id the matrix/scale getters are keyed on, the offset vector and the mode
 * byte that selects the source family. size: 0x50 */
struct _EFT_LIGHT_WORK {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ s32 count_0x18;          /* number of handles at +0x1c */
    /* +0x1c */ MHchar* models_0x1c[5];  /* the pooled light/model handles */
    /* +0x30 */ u8 count2_0x30;          /* the per-frame model count the placement loop runs */
    /* +0x31 */ u8 pad_0x31[3];
    /* +0x34 */ u32 id_0x34;             /* the joint/parameter id the getters take */
    /* +0x38 */ VEC3 vec_0x38;           /* the per-model offset the placement adds */
    /* +0x44 */ f32 scale_0x44;          /* the parameter scale */
    /* +0x48 */ u8 mode_0x48;            /* source family: 0 player, 1 enemy status, 2 enemy action */
    /* +0x49 */ u8 field_0x49;           /* the action/part id passed to the checkers */
    /* +0x4a */ u16 field_0x4a;          /* its second half */
    /* +0x4c */ s16 timer_0x4c;          /* the area-colour re-check countdown */
}; /* size: 0x50 */

/* One per-model record of the frame family: the model the frame is placed on, its three vectors, the
 * two rotation accumulators and their per-frame steps. size: 0x48 */
struct _EFT_FRAME_ENTRY {
    /* +0x00 */ u8 state_0x00;     /* 0 spawn, 1/2 the two placement shapes */
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ MHchar* model_0x04;
    /* +0x08 */ VEC3 vecA_0x08;    /* scaled by the block's +0x98 each frame */
    /* +0x14 */ VEC3 vecB_0x14;    /* rotated by the effect's y angle */
    /* +0x20 */ VEC3 vecC_0x20;    /* the composed placement */
    /* +0x2c */ u32 angle_0x2c;    /* the x/z rotation accumulator */
    /* +0x30 */ u32 angle_0x30;    /* the y rotation accumulator */
    /* +0x34 */ u32 pad_0x34;
    /* +0x38 */ u32 step_0x38;     /* per-frame step of +0x2c */
    /* +0x3c */ u32 step_0x3c;     /* per-frame step of +0x30 */
    /* +0x40 */ u32 step_0x40;     /* seeded but unused by the two handlers below */
    /* +0x44 */ s32 timer_0x44;    /* the per-model frame countdown */
}; /* size: 0x48 */

/* The frame family's block: a count, two fixed records and the two shared per-frame scales.
 * size: 0x9C */
struct _EFT_FRAME_WORK {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ _EFT_FRAME_ENTRY entries[2];
    /* +0x94 */ f32 scale_a_0x94;  /* the per-frame y drop */
    /* +0x98 */ f32 scale_b_0x98;  /* the per-frame vector scale */
}; /* size: 0x9C */

/* The `_PLW` bytes the light family reads that `pl.h` does not name yet (a view; `pl.h` owns the
 * type). size: 0x18 */
struct _EFT_PLW_VIEW {
    /* +0x00 */ u8 slot_active;
    /* +0x01 */ u8 pad_0x01[6];
    /* +0x07 */ u8 field_0x07;  /* the per-frame model count copied into the work block */
    /* +0x08 */ u8 pad_0x08[0x0E];
    /* +0x16 */ u8 area_0x16;
}; /* size: 0x18 */

/* The `_ENEMY_WORK` bytes the light family reads that `enemy/ENEMY_WORK.h` does not name yet (a view;
 * that header owns the type). size: 0x952 */
struct _EFT_ENEMY_VIEW {
    /* +0x000 */ u8 active;
    /* +0x001 */ u8 pad_0x001[0x1A3];
    /* +0x1A4 */ u8 field_0x1a4;  /* the area byte the action family copies onto the effect */
    /* +0x1A5 */ u8 pad_0x1a5[0x3C];
    /* +0x1E1 */ u8 field_0x1e1;  /* the area byte the status family copies onto the effect */
    /* +0x1E2 */ u8 pad_0x1e2[3];
    /* +0x1E5 */ u8 field_0x1e5;  /* the state/action pair the status gate reads */
    /* +0x1E6 */ u8 field_0x1e6;
    /* +0x1E7 */ u8 pad_0x1E7[0x4B];
    /* +0x232 */ s16 field_0x232; /* the action family's hit countdown */
    /* +0x234 */ u8 pad_0x234[0x6E2];
    /* +0x916 */ s16 field_0x916; /* the status family's hit countdown */
    /* +0x918 */ u8 pad_0x918[0x38];
    /* +0x950 */ s16 field_0x950; /* the status family's second countdown */
}; /* size: 0x952 */

/* The object `fn_800E3B8C` hands back, whose owner slot this unit writes. size: 0x30 - lower bound */
struct _EFT_LIGHT_OBJ {
    /* +0x00 */ u8 pad_0x00[0x2C];
    /* +0x2C */ _EFT* owner_0x2C;
}; /* size: 0x30 - lower bound */

/* The `MHchar` model bytes this unit reads past what `pl.h` names. size: 0x11C - lower bound */
struct _EFT_MODEL_VIEW {
    /* +0x000 */ u8 pad_0x000[0x10C];
    /* +0x10C */ void* slot_0x10C;
    /* +0x110 */ u8 pad_0x110[4];
    /* +0x114 */ s32 field_0x114;
    /* +0x118 */ s32 field_0x118;
}; /* size: 0x11C - lower bound */

/* ---------------------------------------------------------------------------------------------------
 * the callees (real C++ declarations - rule 9: never the mangled spelling)
 * ------------------------------------------------------------------------------------------------- */

u32 Pl_act_ck(_PLW* plw, u8 a, u16 b);
s32 em_work_die_ck(_ENEMY_WORK* enemy);
u32 get_now_areano();
nw4r::math::MTX34 get_current_view_mtx();
f32 get_em_scale(_ENEMY_WORK* enemy);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* mtx);
s32 ran_suu(s32 range);
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatZ(u32 angle, nw4r::math::MTX34* mtx);
void rotVecY(nw4r::math::VEC3* v, u32 angle);
void scaleMat34W(nw4r::math::MTX34* mtx, nw4r::math::VEC3* v);

/* The unmangled `fn_XXXXXXXX` callees: inside a linkage block they need no per-declaration spelling. */
extern "C" {
void fn_80041E40(nw4r::math::VEC3* dst, nw4r::math::VEC3* src);
void fn_800504D4(nw4r::math::MTX34* mtx);
void fn_8005050C(nw4r::math::MTX34* mtx);
void fn_800513F0(nw4r::math::VEC3* v, f32 s);
void fn_800532DC(nw4r::math::MTX34* dst, nw4r::math::MTX34* src);
void fn_8007100C(nw4r::math::MTX34* dst, nw4r::math::MTX34* src);
void fn_800710BC(nw4r::math::MTX34* a, nw4r::math::MTX34* b, nw4r::math::MTX34* m);
void fn_80073F68(nw4r::math::VEC3* out, nw4r::math::VEC3* in);
void fn_8007F0CC(s32 root, u32 id);
void fn_800883C4(nw4r::math::MTX34* out, nw4r::math::MTX34* src);
s32 fn_80097EB0(void* handle, s32 index);
s32 fn_8006FDCC(void* handle);
void fn_800DAA4C(nw4r::math::VEC3* pos);
s32 fn_800E0A8C(MHchar* model);
void fn_800E0BE8(MHchar* model, s32 value);
void* fn_800E3B8C(s32 a, u8 b, s32 c, s32 d, s32 e, void (*cb)(void*));
void* fn_800F8788(u32 pool);
void fn_800F886C(void* self);
void* fn_800F8914();
void fn_800F8A44(void* p, s32 mode);
u32 fn_800F93D8(void* self, void* list, s32 a, s32 b, void* c);
u32 fn_800F9D80(void* self);
void fn_800F9DF4(void* self, u8 a, u8 b);
void fn_800FBB90(nw4r::math::MTX34* m, nw4r::math::VEC3* v);
s32 fn_8012E2D4(u32 state, u32 action);
u32 fn_801322CC(_ENEMY_WORK* enemy, s32 value);
u32 fn_80137614(_ENEMY_WORK* enemy);
u32 fn_80137648(_ENEMY_WORK* enemy);
void fn_8026A394(_PLW* plw, s32 id, nw4r::math::MTX34* mtx);
u32 fn_802D2B38(_ENEMY_WORK* enemy, u8 a, u16 b);
void fn_802D2B68(_ENEMY_WORK* enemy, u32 id, nw4r::math::MTX34* mtx);
void fn_8010D29C(void* self);
void fn_8010D2AC(void* self);
f32 fn_8010D1A8(void* self, s32 arg1, f32 farg0);
}

/* The unit's own functions, forward-declared (extern "C" keeps the map's plain spelling). */
extern "C" {
void fn_8010BDE4(_EFT* self);
void fn_8010C0E0(_EFT* self);
void fn_8010C454(_EFT* self);
void fn_8010C464(_EFT* self);
_EFT* fn_8010C468(u32 count);
void fn_8010C554(_PLW* source);
void fn_8010C5DC(void* source, u32 id, nw4r::math::VEC3* vec, f32 scale, u8 type);
void fn_8010C680(void* source);
void fn_8010C710(_EFT* self);
void fn_8010C77C(_EFT* self);
void fn_8010C7B8(_EFT* self);
void fn_8010C8F8(_EFT* self);
void fn_8010CD68(_EFT* self, MHchar** models, s32 count);
void fn_8010CE80(void* arg);
void fn_8010CFCC(_EFT* self, nw4r::math::MTX34* out, nw4r::math::MTX34* mtx, s16 index);
void fn_8010D12C(nw4r::math::MTX34* a, nw4r::math::MTX34* b);
}

/* The shared pools the range references by name (never emitted here; playbook 29). */
extern u8 lbl_80791820[];
extern f32 lbl_8059F518[];
extern f32 lbl_8059F574[];
extern f32 lbl_807967E8;
extern f32 lbl_807967EC;
extern f32 lbl_807967F0;
extern f32 lbl_807967F4;
extern f32 lbl_807967F8;
extern f32 lbl_807967FC;
extern f32 lbl_80796800;
extern f64 lbl_80796808;
extern f32 lbl_80796810;
extern f32 lbl_80796814;
extern f32 lbl_80796818;
extern f32 lbl_8079681C;
extern f64 lbl_80796820;
extern f32 lbl_80796828;
extern f32 lbl_8079682C;
extern f32 lbl_80796830;
extern f32 lbl_80796834;
extern s32 pRoot;

/* ---------------------------------------------------------------------------------------------------
 * the light family
 * ------------------------------------------------------------------------------------------------- */

/* The release hook: frees the effect's pooled handles and clears the count. */
extern "C" void fn_8010C710(_EFT* self)
{
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    s32 i;

    for (i = 0; i < work->count_0x18; i++) {
        fn_800F8A44(&work->models_0x1c[i], 1);
    }

    work->count_0x18 = 0;
}

/* Allocates a light effect with `count` pooled handles, installs its two hooks and clears its pose. */
extern "C" _EFT* fn_8010C468(u32 count)
{
    _EFT* eft = (_EFT*)fn_800F8788(0x50);
    _EFT_LIGHT_WORK* work;
    s32 i;

    if (eft == NULL) {
        return NULL;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    work->count_0x18 = (u8)count;
    eft->dispatch_0x34 = fn_8010C77C;
    eft->release_0x40 = fn_8010C710;

    for (i = 0; i < work->count_0x18; i++) {
        work->models_0x1c[i] = (MHchar*)fn_800F8914();
        if (work->models_0x1c[i] == NULL) {
            fn_800F886C(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 0x10;
    eft->rot_0x24.z = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.x = 0;
    eft->timer_0x0C = 0;
    eft->flag_0x01 = 1;
    return eft;
}

/* The five-light player hit effect: no offset, a full second of life. */
extern "C" void fn_8010C554(_PLW* source)
{
    _EFT* eft = fn_8010C468(5);
    _EFT_LIGHT_WORK* work;

    if (eft == NULL) {
        return;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    eft->type_0x02 = 0;
    eft->source_0x30 = source;
    work->mode_0x48 = 0;
    work->id_0x34 = 0xD;
    work->vec_0x38.x = lbl_80796810;
    work->vec_0x38.y = lbl_80796810;
    work->vec_0x38.z = lbl_80796810;
    work->scale_0x44 = lbl_80796814;
    eft->area_0x44 = source->area_0x16;
    work->count2_0x30 = 5;
    fn_800F9DF4(eft, 1, 0);
}

/* The parameterised one-light player effect. */
extern "C" void fn_8010C5DC(void* source, u32 id, nw4r::math::VEC3* vec, f32 scale, u8 type)
{
    _EFT* eft = fn_8010C468(1);
    _EFT_LIGHT_WORK* work;

    if (eft == NULL) {
        return;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    eft->type_0x02 = type;
    eft->source_0x30 = source;
    work->mode_0x48 = 1;
    work->id_0x34 = id;
    fn_80041E40(&work->vec_0x38, vec);
    work->scale_0x44 = scale;
    eft->area_0x44 = ((_EFT_ENEMY_VIEW*)source)->field_0x1e1;
    work->count2_0x30 = 5;
    fn_800F9DF4(eft, 1, 0);
}

/* The five-light player effect with a raised offset. */
extern "C" void fn_8010C680(void* source)
{
    _EFT* eft = fn_8010C468(5);
    _EFT_LIGHT_WORK* work;

    if (eft == NULL) {
        return;
    }

    work = (_EFT_LIGHT_WORK*)eft->work_0x38;
    eft->type_0x02 = 0;
    eft->source_0x30 = source;
    work->mode_0x48 = 2;
    work->id_0x34 = 0xE;
    work->vec_0x38.x = lbl_80796810;
    work->vec_0x38.y = lbl_80796818;
    work->vec_0x38.z = lbl_80796810;
    work->scale_0x44 = lbl_80796814;
    eft->area_0x44 = ((_EFT_ENEMY_VIEW*)source)->field_0x1a4;
    work->count2_0x30 = 5;
    fn_800F9DF4(eft, 1, 0);
}

/* The state dispatcher: the first two states are this unit's, the last two the next unit's. */
extern "C" void fn_8010C77C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_8010C7B8(self);
        break;
    case 1:
        fn_8010C8F8(self);
        break;
    case 2:
        fn_8010D29C(self);
        break;
    case 3:
        fn_8010D2AC(self);
        break;
    }
}

/* State 0: creates each pooled light's model from the type's id table and seats it on the source. */
extern "C" void fn_8010C7B8(_EFT* self)
{
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    s32 i;

    self->state_0x05++;

    if (self->type_0x02 >= 4) {
        fn_8010D2AC(self);
        return;
    }

    for (i = 0; i < work->count_0x18; i++) {
        MHchar* model = work->models_0x1c[i];
        if (res_eft_model_create(model, lbl_80791820[self->type_0x02], 0) == NULL) {
            fn_8010D2AC(self);
            return;
        }
        model->field_0x30 = 0;
        model->field_0x2C = 0;
        model->field_0x28 = 0;
        setVector3(&model->scale_0x1C, lbl_80796814, lbl_80796814, lbl_80796814);
    }

    if (self->source_0x30 == NULL) {
        self->state_0x05 = 2;
        return;
    }

    switch (work->mode_0x48) {
    case 0:
        work->field_0x49 = ((_EFT_PLW_VIEW*)self->source_0x30)->field_0x07;
        work->field_0x4a = ((_EFT_PLW_VIEW*)self->source_0x30)->pad_0x08[0];
        break;
    case 2:
        work->field_0x49 = ((_EFT_ENEMY_VIEW*)self->source_0x30)->field_0x1e1;
        work->field_0x4a = ((_EFT_ENEMY_VIEW*)self->source_0x30)->pad_0x1e2[0];
        work->timer_0x4c = 0;
        break;
    }

    fn_8010C8F8(self);
}

/* State 1: the per-frame placement. */
extern "C" void fn_8010C8F8(_EFT* self)
{
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    VEC3 vec;
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    _EFT_PLW_VIEW* plw;
    _EFT_ENEMY_VIEW* em;
    f32 f31;
    s32 i;

    fn_8005050C(&mtxA);
    fn_8005050C(&mtxB);
    fn_80043EA8(&vec);

    if (self->source_0x30 == NULL) {
        self->state_0x05++;
        return;
    }

    plw = (_EFT_PLW_VIEW*)self->source_0x30;
    em = (_EFT_ENEMY_VIEW*)self->source_0x30;

    switch (work->mode_0x48) {
    case 0:
        if (plw->slot_active == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        if (Pl_act_ck((_PLW*)self->source_0x30, work->field_0x49, work->field_0x4a) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        fn_8026A394((_PLW*)self->source_0x30, work->id_0x34, &mtxA);
        f31 = lbl_80796814;
        work->count2_0x30 = plw->field_0x07;
        self->area_0x44 = plw->area_0x16;
        break;
    case 1:
        if (em_work_die_ck((_ENEMY_WORK*)self->source_0x30) == 1) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        switch (self->type_0x02) {
        case 0:
            if (((fn_801322CC((_ENEMY_WORK*)self->source_0x30, 0x10) == 1 &&
                  fn_8012E2D4(em->field_0x1e5, em->field_0x1e6) == 1 && em->field_0x950 > 0) ||
                 fn_80137614((_ENEMY_WORK*)self->source_0x30) == 1 ||
                 fn_80137648((_ENEMY_WORK*)self->source_0x30) == 1)) {
                break;
            }
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        case 1:
            if (em->field_0x916 > 0) {
                break;
            }
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        case 2:
            if (((fn_801322CC((_ENEMY_WORK*)self->source_0x30, 0x10) == 1 &&
                  fn_8012E2D4(em->field_0x1e5, em->field_0x1e6) == 1 && em->field_0x950 > 0) ||
                 fn_80137614((_ENEMY_WORK*)self->source_0x30) == 1 ||
                 fn_80137648((_ENEMY_WORK*)self->source_0x30) == 1)) {
                break;
            }
            self->flag_0x01 = 0;
            return;
        case 3:
            if (em->field_0x916 > 0) {
                break;
            }
            self->flag_0x01 = 0;
            return;
        }
        get_joint_wmat_em((_ENEMY_WORK*)self->source_0x30, work->id_0x34, &mtxA);
        f31 = get_em_scale((_ENEMY_WORK*)self->source_0x30);
        self->area_0x44 = em->field_0x1e1;
        break;
    case 2:
        if (em->active != 0 &&
            fn_802D2B38((_ENEMY_WORK*)self->source_0x30, work->field_0x49, work->field_0x4a) != 0 &&
            em->field_0x232 > 0) {
            fn_802D2B68((_ENEMY_WORK*)self->source_0x30, work->id_0x34, &mtxA);
            f31 = lbl_80796814;
            self->area_0x44 = em->field_0x1a4;
            break;
        }
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }

    self->flag_0x01 = 1;
    self->rot_0x24.y = (u32)((f32)self->rot_0x24.y + lbl_8079681C);

    if (self->type_0x02 >= 2) {
        MHchar* model = work->models_0x1c[0];
        fn_800E0BE8(model, 1);
        model->move(0);
        fn_8010CD68(self, work->models_0x1c, work->count_0x18);
    } else {
        for (i = 0; i < work->count2_0x30; i++) {
            MHchar* model = work->models_0x1c[i];
            f32 s;
            fn_8010CFCC(self, &mtxB, &mtxA, i);
            fn_8010D12C(&mtxB, &mtxB);
            s = fn_8010D1A8(self, i, f31);
            setVector3(&model->scale_0x1C, s, s, s);
            fn_800E0BE8(model, 1);
            model->move2(&mtxB, 0);
        }
    }

    fn_800F93D8(self, &work->models_0x1c[0], 2, work->count2_0x30, NULL);
    self->timer_0x0C++;
    if (self->timer_0x0C >= 0x20) {
        self->timer_0x0C = 0;
        if (work->mode_0x48 == 0) {
            work->timer_0x4c--;
            if (work->timer_0x4c <= 0) {
                work->timer_0x4c = 0x2D;
                if (self->area_0x44 == get_now_areano()) {
                    setVector3(&vec, mtxA.m[0][3], mtxA.m[1][3], mtxA.m[2][3]);
                    fn_800DAA4C(&vec);
                }
            }
        }
    }
}

/* The light-object callback `fn_800E3B8C` installs: places the effect's models on the enemy joint. */
extern "C" void fn_8010CE80(void* arg)
{
    _EFT* self = ((_EFT_LIGHT_OBJ*)arg)->owner_0x2C;
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    _ENEMY_WORK* enemy = (_ENEMY_WORK*)self->source_0x30;
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    f32 f31;
    s16 i;

    fn_8005050C(&mtxA);
    fn_8005050C(&mtxB);

    if (enemy != NULL && work->mode_0x48 == 1) {
        if (em_work_die_ck(enemy) == 1) {
            return;
        }
        get_joint_wmat_em(enemy, work->id_0x34, &mtxA);
        f31 = get_em_scale(enemy);
    }

    for (i = 0; i < work->count2_0x30; i++) {
        VEC3 scale;
        f32 s;
        s32 base;
        s32 handle;
        s32 idx;
        fn_8010CFCC(self, &mtxB, &mtxA, i);
        fn_8010D12C(&mtxB, &mtxB);
        s = fn_8010D1A8(self, i, f31);
        fn_80041E8C((Vec*)&scale, s, s, s);
        scaleMat34W(&mtxB, &scale);
        base = fn_800E0A8C((MHchar*)((_EFT_MODEL_VIEW*)work->models_0x1c[i])->field_0x118);
        handle = fn_80097EB0(&((_EFT_MODEL_VIEW*)work->models_0x1c[i])->field_0x114, i + 1);
        idx = fn_8006FDCC(&handle);
        fn_8007100C(&((nw4r::math::MTX34*)base)[idx], &mtxB);
    }
}

/* The per-model matrix builder: rotates the model's offset by its angle and composes the effect
 * matrix into `out`. */
extern "C" void fn_8010CFCC(_EFT* self, nw4r::math::MTX34* out, nw4r::math::MTX34* mtx, s16 index)
{
    _EFT_LIGHT_WORK* work = (_EFT_LIGHT_WORK*)self->work_0x38;
    VEC3 v20;
    VEC3 v14;
    VEC3 v8;
    f32 angle;

    fn_80043EA8(&v20);
    fn_80043EA8(&v14);
    fn_80041E40(&v14, &work->vec_0x38);
    mulVecMat(&v14, mtx);
    fn_80041E8C((Vec*)&v8, lbl_80796810, lbl_80796810, work->scale_0x44 * lbl_8059F518[index]);
    fn_80041E40(&v20, &v8);
    rotVecY(&v20, self->rot_0x24.y + index * 0x3333);
    angle = lbl_8079682C * lbl_8059F574[index];
    angle = angle / lbl_80796830;
    angle = lbl_80796828 + angle;
    rotVecZ(&v20, (u16)(s32)angle);
    v20.y = v20.y + lbl_80796834;
    mulVecMat(&v20, mtx);
    fn_800504D4(out);
    out->m[0][3] = v20.x + mtx->m[0][3] + v14.x;
    out->m[1][3] = v20.y + mtx->m[1][3] + v14.y;
    out->m[2][3] = v20.z + mtx->m[2][3] + v14.z;
}

/* Applies the current view matrix to the two matrices (the per-model transform). */
extern "C" void fn_8010D12C(nw4r::math::MTX34* a, nw4r::math::MTX34* b)
{
    nw4r::math::MTX34 m;

    fn_8005050C(&m);
    fn_800532DC(&m, &get_current_view_mtx());
    m.m[0][3] = lbl_80796810;
    m.m[1][3] = lbl_80796810;
    m.m[2][3] = lbl_80796810;
    fn_800883C4(&m, &m);
    fn_800710BC(a, b, &m);
}

/* Places each pooled model: either the two-model enemy case or the per-model light case. */
extern "C" void fn_8010CD68(_EFT* self, MHchar** models, s32 count)
{
    VEC3 vec;
    s32 i;

    fn_80043EA8(&vec);

    if ((u8)self->area_0x44 != (u8)get_now_areano()) {
        return;
    }

    if (self->flag_0x01 == 0) {
        for (i = 0; i < count; i++) {
            MHchar* model = models[i];
            if (model == NULL) {
                continue;
            }
            if (((_EFT_MODEL_VIEW*)model)->field_0x118 ==
                *(u32*)((u8*)((_EFT_MODEL_VIEW*)model)->slot_0x10C + 4)) {
                fn_8007F0CC(pRoot, 0);
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            MHchar* model = models[i];
            void* p;
            if (model == NULL) {
                continue;
            }
            if (((_EFT_MODEL_VIEW*)model)->field_0x118 !=
                *(u32*)((u8*)((_EFT_MODEL_VIEW*)model)->slot_0x10C + 4)) {
                continue;
            }
            p = fn_800E3B8C(1, model->ready, 0, 0x40, 0, fn_8010CE80);
            if (p != NULL) {
                ((_EFT_LIGHT_OBJ*)p)->owner_0x2C = self;
                fn_8007F0CC(pRoot, ((_EFT_MODEL_VIEW*)model)->field_0x118);
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * the frame family
 * ------------------------------------------------------------------------------------------------- */

/* Seeds the two per-model frame records and hands off to the placement handler. */
extern "C" void fn_8010BDE4(_EFT* self)
{
    _EFT_FRAME_WORK* work = (_EFT_FRAME_WORK*)self->work_0x38;
    nw4r::math::MTX34 mtx;
    f32 scale;
    f32 sa;
    f32 sb;
    f32 neg;
    s32 base_a;
    s32 base_b;
    s32 base_c;
    s32 i;

    fn_8005050C(&mtx);

    self->state_0x05++;
    scale = (fn_800F9D80(self) == 1) ? lbl_807967E8 : lbl_807967EC;
    neg = lbl_807967F8;
    sa = lbl_807967FC * scale;
    sb = lbl_80796800 * scale;
    base_a = 0x300;
    base_b = 0x900;
    base_c = 0x400;

    for (i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create(work->entries[i].model_0x04, 0x1A, 4) == NULL) {
            fn_8010C464(self);
            return;
        }
        setVector3(&work->entries[i].model_0x04->scale_0x1C, lbl_807967EC, lbl_807967EC, lbl_807967EC);
        setVector3(&work->entries[i].vecB_0x14, lbl_807967F0, lbl_807967F4, lbl_807967F4);
        if (self->type_0x02 == 0) {
            u16 r = (u16)ran_suu(0);
            work->entries[i].timer_0x44 = (r & 3);
            work->entries[i].timer_0x44 = work->entries[i].timer_0x44 + 1;
        } else {
            work->entries[i].timer_0x44 = 0;
        }
        if (i == 1) {
            work->entries[i].vecB_0x14.x = neg * work->entries[i].vecB_0x14.x;
            work->entries[i].vecB_0x14.x = work->entries[i].vecB_0x14.x - (f32)((u16)ran_suu(0) % 8);
        } else {
            work->entries[i].vecB_0x14.x = work->entries[i].vecB_0x14.x + (f32)((u16)ran_suu(0) % 8);
        }
        work->entries[i].vecB_0x14.y = (f32)((u16)ran_suu(0) % 15);
        work->entries[i].step_0x38 = base_a + ((u16)ran_suu(0) % 512);
        work->entries[i].step_0x3c = base_b + ((u16)ran_suu(0) % 512);
        work->entries[i].step_0x40 = base_c + ((u16)ran_suu(0) % 512);
        fn_800513F0(&work->entries[i].vecB_0x14, scale);
        work->scale_a_0x94 = sa;
        work->scale_b_0x98 = sb;
    }

    fn_8010C0E0(self);
}

/* Places each frame record's model along its angles for the record's state. */
extern "C" void fn_8010C0E0(_EFT* self)
{
    _EFT_FRAME_WORK* work = (_EFT_FRAME_WORK*)self->work_0x38;
    nw4r::math::MTX34 mtx;
    VEC3 vec;
    s32 i;

    fn_8005050C(&mtx);
    fn_80043EA8(&vec);

    if (work->entries[0].state_0x00 == 3 && work->entries[1].state_0x00 == 3) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }

    for (i = 0; i < work->count_0x00; i++) {
        work->entries[i].timer_0x44--;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        if (i == 1) {
            rotLocalMatZ(0x8000, &mtx);
        }
        switch (work->entries[i].state_0x00) {
        case 0:
            if (self->source_0x30 == NULL) {
                work->entries[i].timer_0x44 = 0;
            } else if (*(u8*)self->source_0x30 != 0) {
                work->entries[i].timer_0x44 = 0;
            }
            if (work->entries[i].timer_0x44 <= 0) {
                work->entries[i].timer_0x44 = 5;
                work->entries[i].state_0x00++;
                rotVecY(&work->entries[i].vecB_0x14, self->rot_0x24.y);
                rotLocalMatY(work->entries[i].angle_0x30, &mtx);
                fn_80073F68(&work->entries[i].vecC_0x20, &work->entries[i].vecB_0x14);
                fn_800FBB90(&mtx, &work->entries[i].vecC_0x20);
                work->entries[i].model_0x04->move2(&mtx, 0);
                fn_800F93D8(self, &work->entries[i].model_0x04, 2, 1, NULL);
            } else {
                fn_800FBB90(&mtx, &work->entries[i].vecC_0x20);
                work->entries[i].model_0x04->move2(&mtx, 0);
                fn_80073F68(&work->entries[i].vecC_0x20, &work->entries[i].vecA_0x08);
                fn_800F93D8(self, &work->entries[i].model_0x04, 2, 1, NULL);
            }
            break;
        case 1:
            rotLocalMatY(work->entries[i].angle_0x30, &mtx);
            mtx.m[0][3] = work->entries[i].vecC_0x20.x;
            mtx.m[1][3] = work->entries[i].vecC_0x20.y;
            mtx.m[2][3] = work->entries[i].vecC_0x20.z;
            if (work->entries[i].timer_0x44 <= 0) {
                work->entries[i].timer_0x44 = 0xF;
                work->entries[i].state_0x00++;
                work->entries[i].model_0x04->move2(&mtx, 0);
                work->entries[i].vecB_0x14.y = work->entries[i].vecB_0x14.y - work->scale_a_0x94;
                fn_800513F0(&work->entries[i].vecA_0x08, work->scale_b_0x98);
                fn_80073F68(&work->entries[i].vecC_0x20, &work->entries[i].vecB_0x14);
                fn_80073F68(&work->entries[i].vecC_0x20, &work->entries[i].vecA_0x08);
                work->entries[i].angle_0x30 = work->entries[i].angle_0x30 + work->entries[i].step_0x3c;
                fn_800F93D8(self, &work->entries[i].model_0x04, 2, 1, NULL);
            }
            break;
        case 2:
            rotLocalMatZ(work->entries[i].angle_0x2c, &mtx);
            rotLocalMatX(work->entries[i].angle_0x2c, &mtx);
            rotLocalMatY(work->entries[i].angle_0x30, &mtx);
            mtx.m[0][3] = work->entries[i].vecC_0x20.x;
            mtx.m[1][3] = work->entries[i].vecC_0x20.y;
            mtx.m[2][3] = work->entries[i].vecC_0x20.z;
            if (work->entries[i].timer_0x44 <= 0) {
                work->entries[i].state_0x00++;
                work->entries[i].model_0x04->move2(&mtx, 0);
                work->entries[i].vecB_0x14.y = work->entries[i].vecB_0x14.y - work->scale_a_0x94;
                fn_800513F0(&work->entries[i].vecA_0x08, work->scale_b_0x98);
                fn_80073F68(&work->entries[i].vecC_0x20, &work->entries[i].vecB_0x14);
                fn_80073F68(&work->entries[i].vecC_0x20, &work->entries[i].vecA_0x08);
                work->entries[i].angle_0x30 = work->entries[i].angle_0x30 + work->entries[i].step_0x3c;
                work->entries[i].angle_0x2c = work->entries[i].angle_0x2c + work->entries[i].step_0x38;
                fn_800F93D8(self, &work->entries[i].model_0x04, 2, 1, NULL);
            }
            break;
        }
    }
}

/* The player-side state hook the next unit's dispatcher tail-calls. */
extern "C" void fn_8010C454(_EFT* self)
{
    self->state_0x05++;
}

/* The delete thunk the light family installs as a pool destructor. */
extern "C" void fn_8010C464(_EFT* self)
{
    fn_800F886C(self);
}
