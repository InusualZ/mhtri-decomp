/* auto/8010D1A8_fn_8010D1A8.c - the player action-effect state machine, `.text` 0x8010D1A8..0x801121DC.
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * What it is.  The unit drives the nw4r effects a player action spawns: an `Eft` record carries a
 * `state_0x05` the per-variant handler advances and a `type_0x02` selecting the variant body, and its
 * `EftWork` block holds the nw4r effect handle (`effect`), its colour and scale, the per-variant light
 * handles and the joint matrix.  The entry points are the spawn helpers (`fn_8010D2B0`, `fn_8010D608`,
 * `fn_8010D678`, `fn_8010D688`), which build the record through `fn_800F8788`/`fn_8010D70C` and install
 * `fn_8010D3C4`/`fn_8010D8B0` as the update hook; the variants then create effects, lights and colours
 * through `res_eft_create`/`res_eft_model_create_light` and the `change_*_eff` helpers.  The day-cycle
 * interpolation in `fn_8010D1A8` scales the effect over the keyframe table `lbl_8059F530`.
 *
 * Source shapes worth keeping (each measured against the target):
 *   * `fn_8010D400` and the `fn_800F8788` handle store need `#pragma peephole off` (playbook 39) - the
 *     pool-block store has to keep the target's reload; the pragma is scoped to that one function.
 *   * the colour split in `fn_8010D400` is written with explicit `(color & mask) >> shift` terms; the
 *     `(color >> shift) & 0xFF` form folds into one `rlwinm` and loses the target's three-instruction
 *     window.
 *   * `fn_8010D2B0`'s areano parameter is `s8` compared as `(u8)arg1`; `fn_8010D70C`'s last two are `s32`
 *     narrowed with `(u8)` - the mask is in the target and only appears from the cast.
 *   * `fn_8010D8B0`'s outer switch lists case 0 last so MWCC emits the per-state tail calls before the
 *     nested per-type switch (the compare chain is then 1,2,3,0 where retail has 0,1,2,3).
 *
 * Residuals.  `fn_8010D1A8` (61.90 %) keeps the table base and index in the wrong registers and
 * materialises the int-to-float magic from the unit's own pool where the target references the shared
 * `lbl_80796840`.  `fn_8010D8B0` (93.20 %) differs only in the outer switch's compare order.
 * `fn_8010D70C` (96.71 %) misses the callback's `addi r0` form and the loop's reload (the peephole
 * pragma that fixes the reload costs the type store's mask, net negative).
 *
 * Unrecovered.  Eight functions are still the original bytes: `fn_8010DA48` (0x3F8), `fn_8010E044`,
 * `fn_8010E2A8` (the 0x2770-byte jump-table switch), `fn_80110A18`, `fn_80110FE0`, `fn_80111344`,
 * `fn_80111870` and `fn_80112000`.
 *
 * Data.  The unit owns no pool section: its `.sdata2` floats and `.sdata`/`.data` tables live in a shared
 * pool, so they are `extern`-declared by their map names and never defined (playbook 29).  `lbl_8059F530`
 * is the day-cycle keyframe array, `lbl_8059F560` the per-move position offsets, `lbl_8059F588`/
 * `lbl_8059F59C` the per-type effect/parameter ids, `lbl_80791838` the per-item light ids, `lbl_8059F5B0`/
 * `lbl_8059F5C8` the colour tables and `lbl_80591828` the light colour.
 *
 * Types.  `Eft`, `EftWork`, `EftLight`, `EftModel` and `Plw` are reconstructed from the field offsets in
 * `.text` (no DWARF in an MWCC object); the overlapping views (`EftRot`, `EftWorkSlots`, `EftWorkBody`)
 * are unions because the variants reuse the same bytes for different payloads.
 *
 * Language.  The unit's own symbols are plain, so the file stays C and the mangled callees are declared
 * by their map spelling, as `auto/800FD520_fn_800FD520.c` does.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8010D1A8_fn_8010D1A8.c`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "unsplit/g3d.h"
#include "unsplit/sound.h"
#include "ef/eft004.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* ---- math types ---- */
/* `Vec3` (and the `VEC3`/`MTX34`/`Mtx34` spellings) come from `nw4r/math.h` - one definition, in the
 * owner's header (rule 1). */

typedef struct CPMtxVec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} CPMtxVec; /* size: 0x0C */

/* ---- the effect record ---- */
struct EftWork;

/* The rotation vector at +0x24: cpSetRotMatrix reads it as three floats, while the spawn path
 * writes its middle word as an integer. size: 0x0C */
typedef union EftRot {
    /* +0x00 */ CPMtxVec vec;
    /* +0x00 */ struct {
        /* +0x00 */ u8 pad_0x00[4];
        /* +0x04 */ s32 field_0x28;
    } raw; /* size: 0x0C */
} EftRot; /* size: 0x0C */

typedef struct Eft {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 kind_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 unused_0x06;
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ u8 demo_flag_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 field_0x0C;
    /* +0x10 */ u8 unused_0x10[0x18 - 0x10];
    /* +0x18 */ Vec3 pos_0x18;
    /* +0x24 */ EftRot rot_0x24;
    /* +0x30 */ void* source_0x30;
    /* +0x34 */ void (*cb_0x34)(struct Eft*);
    /* +0x38 */ struct EftWork* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*cb_0x40)(struct Eft*);
    /* +0x44 */ u8 areano_0x44;
    /* +0x45 */ u8 unused_0x45[0x60 - 0x45];
    /* +0x60 */ f32 field_0x60;
    /* +0x64 */ u8 unused_0x64[0x70 - 0x64];
} Eft; /* size: 0x70 - lower bound (allocated size) */

/* The colour the work block carries at +0x08, written per channel and read back whole. size: 0x04 */
typedef union EftColor {
    /* +0x00 */ u32 rgba;
    /* +0x00 */ struct {
        /* +0x00 */ u8 r;
        /* +0x01 */ u8 g;
        /* +0x02 */ u8 b;
        /* +0x03 */ u8 a;
    } c; /* size: 0x04 */
} EftColor; /* size: 0x04 */

/* The light object fn_800E3B8C hands back; its owner pointer is the only field this unit writes.
 * size: 0x30 - lower bound. */
typedef struct EftLightObj {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ Eft* owner_0x2C;
} EftLightObj; /* size: 0x30 - lower bound */

/* The slot record the model's field at +0x10C points at. size: 0x08 - lower bound */
typedef struct EftModelSlot {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ u32 field_0x4;
} EftModelSlot; /* size: 0x08 - lower bound */

/* The model object the light effect is attached to; only the fields this unit reads are named.
 * size: 0x11C - lower bound. */
typedef struct EftModel {
    /* +0x000 */ u8 unused_0x000[0x35];
    /* +0x035 */ u8 field_0x35;
    /* +0x036 */ u8 unused_0x036[0x10C - 0x36];
    /* +0x10C */ EftModelSlot* field_0x10C;
    /* +0x110 */ u8 unused_0x110[0x118 - 0x110];
    /* +0x118 */ u32 field_0x118;
} EftModel; /* size: 0x11C - lower bound */

/* A three-channel colour entry of the effect-colour tables. size: 0x03 */
typedef struct Rgb3 {
    /* +0x00 */ u8 r;
    /* +0x01 */ u8 g;
    /* +0x02 */ u8 b;
} Rgb3; /* size: 0x03 */

/* One of the sub-effect's light handles: the nw4r light object the effect attaches to a joint.
 * size: 0x40 - lower bound, only the fields this unit touches are named. */
typedef struct EftLight {
    /* +0x00 */ u8 unused_0x00[0x1C];
    /* +0x1C */ Vec3 pos_0x1C;
    /* +0x28 */ u8 unused_0x28[0x35 - 0x28];
    /* +0x35 */ u8 field_0x35;
    /* +0x36 */ u8 unused_0x36[0x40 - 0x36];
} EftLight; /* size: 0x40 - lower bound */

/* The work block's slot array at +0x10: the light handles the type-0 body walks, or the effect handles
 * the teardown frees. size: 0x1C */
typedef union EftWorkSlots {
    /* +0x00 */ struct {
        /* +0x00 */ EftLight* items_0x10[5];
    } items; /* size: 0x14 */
    /* +0x00 */ struct {
        /* +0x00 */ u8 pad_0x10[8];
        /* +0x08 */ void* handles_0x18[5];
    } handles; /* size: 0x1C */
} EftWorkSlots; /* size: 0x1C */

/* The work block's payload at +0x34: a joint matrix for the follow effects, a scale for the day-cycle
 * effect. size: 0x30 */
typedef union EftWorkBody {
    /* +0x00 */ Mtx34 matrix_0x34;
    /* +0x00 */ struct {
        /* +0x00 */ u8 pad_0x00[0x10];
        /* +0x10 */ f32 scale_0x44;
    } as_scaled; /* size: 0x30 */
} EftWorkBody; /* size: 0x30 */

typedef struct EftWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ void* effect;
    /* +0x08 */ EftColor color_0x08;
    /* +0x0C */ f32 scale_0x0C;
    /* +0x10 */ EftWorkSlots slots_0x10;
    /* +0x2C */ u16 field_0x2C;
    /* +0x2E */ u8 field_0x2E;
    /* +0x2F */ s8 field_0x2F;
    /* +0x30 */ u8 unused_0x30[0x34 - 0x30];
    /* +0x34 */ EftWorkBody body_0x34;
    /* +0x64 */ u8 field_0x64;
    /* +0x65 */ u8 unused_0x65[0x67 - 0x65];
    /* +0x67 */ u8 field_0x67;
    /* +0x68 */ f32 field_0x68;
    /* +0x6C */ f32 field_0x6C;
} EftWork; /* size: 0x70 - lower bound */

/* A day-cycle keyframe: the position the segment starts at and the scale reached there. size: 0x08 */
typedef struct EftDayKey {
    /* +0x00 */ f32 pos;
    /* +0x04 */ f32 scale;
} EftDayKey; /* size: 0x08 */

/* The player actor the effect was spawned for; only the fields this unit reads are named.
 * size: 0x668 - lower bound, the full type is Pl/pl_act.cpp's `_PLW`. */
typedef struct Plw {
    /* +0x000 */ u8 unused_0x000[0x02];
    /* +0x002 */ u8 weapon_class_0x02;
    /* +0x003 */ u8 unused_0x003[0x08 - 0x03];
    /* +0x008 */ s32 field_0x08;
    /* +0x00C */ u8 unused_0x00C[0x16 - 0x0C];
    /* +0x016 */ u8 field_0x16;
    /* +0x017 */ u8 unused_0x017[0x60 - 0x17];
    /* +0x060 */ f32 field_0x60;
    /* +0x064 */ u8 unused_0x064[0x7C - 0x64];
    /* +0x07C */ f32 field_0x7C;
    /* +0x080 */ u8 unused_0x080[0x13C - 0x80];
    /* +0x13C */ u8* physics_0x13C;
    /* +0x140 */ u8 unused_0x140[0x668 - 0x140];
} Plw; /* size: 0x668 - lower bound */

/* ---- externs ---- */
extern u8 get_now_areano__Fv(void);
extern u32 event_demo_ck__Fv(void);
extern void* fn_800F8788(u32 size);
extern void fn_800F886C(Eft* eft);
extern void fn_800F9DF4(Eft* eft, s32 a, s32 b);
extern void push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(void** effect, s32 n);
extern void cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(CPMtxVec* rot, Mtx34* mtx);
extern void SetRootMtx__Q34nw4r2ef6EffectFRCQ34nw4r4math5MTX34(void* effect, Mtx34* mtx);
extern void* res_eft_create__FUsUsUl(u16 id, u16 param, u32 a);
extern u32 get_stg_eft_col__FUcUc(u8 areano, u8 idx);
extern void change_paramscale_eff__FPQ34nw4r2ef6Effectf(void* effect, f32 scale);
extern s32 effect_move__FPQ34nw4r2ef6Effect(void* effect);
extern void change_color_eff__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC38_GXColor(void* effect, Vec3* pos, u32* color);
extern void fn_800F93D8(Eft* self, void* effect, s32 a, s32 b, s32 c);

extern u16 lbl_8059F588[10];
extern u16 lbl_8059F59C[10];
extern u32 lbl_8059F560[];
extern EftDayKey lbl_8059F530[];
extern f32 lbl_80796838; /* -1.0f, the day-key terminator */
extern f64 lbl_80796840; /* the int-to-double magic */

extern EftLight* fn_800F8914(void);
extern void fn_800F8A44(void* p, s32 n);
extern u8 fn_800CF208(void);
extern Eft* fn_8010D70C(Plw* source, s32 arg1, s32 arg2, s32 arg3);
extern void fn_8010D824(Eft* self);
extern void fn_8010D8B0(Eft* self);
extern void fn_8010D928(Eft* self);
extern void fn_8010DA48(Eft* self);
extern void fn_8010DE40(Eft* self);
extern void fn_8010DF38(Eft* self);
extern void fn_8010E008(Eft* self);
extern void fn_80111624(Eft* self);
extern void fn_80111634(Eft* self);
extern void fn_8010E2A8(Eft* self);
extern void fn_80110A18(Eft* self);
extern void fn_80110FE0(Eft* self);
extern void fn_80111344(Eft* self);
extern s32 res_eft_model_create_light__FP6MHcharUsUll(EftLight* light, u16 id, u32 a, s32 b);
extern void setVector3__FPQ34nw4r4math4VEC3fff(Vec3* v, f32 x, f32 y, f32 z);
extern s16 fn_802BF814(void);
extern void fn_80111870(void);
extern s32 fn_8026A328(s32 a, f32 b, f32 c);
extern s32 pRoot;
extern u16 Get_motion_no__FP4_PLW(Plw* plw);
extern s32 Pl_frame_check__FP4_PLWUlff(Plw* plw, u32 a, f32 b, f32 c);
extern Rgb3 lbl_8059F5B0[];
extern Rgb3 lbl_8059F5C8[];
extern f32 lbl_80796884;
extern f32 lbl_8079688C;
extern f32 lbl_8079689C;
extern f32 lbl_807968B0;
extern f32 lbl_8079690C;
extern f32 lbl_80796910;
extern f32 lbl_8079692C;
extern f32 lbl_80796934;
extern f32 lbl_807969D0;
extern f32 lbl_807969D4;
extern u16 lbl_80791838[];
extern f32 lbl_80796848; /* 1.0f */
extern void setVisibility__6MHcharFUlb(void* mhchar, u32 id, u32 visible);

void fn_8010D388(Eft* self);
void fn_8010D3C4(Eft* self);
void fn_8010D400(Eft* self);
void fn_8010D50C(Eft* self);
void fn_8010D5F4(Eft* self);
void fn_8010D604(Eft* self);

void fn_8010D29C(Eft* self)
{
    self->state_0x05++;
}

/* Interpolates the effect scale over the day-cycle keyframes and applies the record's own scale. */
f32 fn_8010D1A8(Eft* self, s32 arg1, f32 farg0)
{
    s32 idx;
    EftWork* work;
    EftDayKey* key;
    s32 pos;
    s32 i;

    work = self->work_0x38;
    key = lbl_8059F530;
    pos = self->field_0x0C + lbl_8059F560[(s16)arg1];
    if (pos >= 32) {
        pos -= 32;
    }
    idx = self->field_0x0C;
    for (i = 1; key[i].pos != lbl_80796838; i++) {
        if ((f32)pos <= key[i].pos) {
            idx = i - 1;
            break;
        }
    }
    key = &lbl_8059F530[idx];
    return work->body_0x34.as_scaled.scale_0x44 * (key->scale + ((key[1].scale - key->scale) / (key[1].pos - key->pos)) * ((f32)pos - key->pos)) * farg0;
}

void fn_8010D2AC(Eft* self)
{
    fn_800F886C(self);
}

void fn_8010D2B0(void* arg0, s8 arg1, s8 arg2, u32 arg3, f32 farg0)
{
    Eft* eft;
    EftWork* work;

    if ((u8)arg1 == get_now_areano__Fv()) {
        eft = fn_800F8788(0x10);
        if (eft != NULL) {
            work = eft->work_0x38;
            work->count = 1;
            work->scale_0x0C = farg0;
            eft->kind_0x03 = 0x11;
            eft->type_0x02 = arg2;
            copyVec3(&eft->pos_0x18, arg0);
            eft->rot_0x24.raw.field_0x28 = arg3;
            eft->areano_0x44 = arg1;
            eft->cb_0x40 = fn_8010D388;
            eft->cb_0x34 = fn_8010D3C4;
            fn_800F9DF4(eft, 0, 0);
            if (event_demo_ck__Fv() == 1) {
                eft->demo_flag_0x08 = 1;
            }
        }
    }
}

void fn_8010D388(Eft* self)
{
    EftWork* work;

    work = self->work_0x38;
    push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(&work->effect, work->count);
    work->count = 0;
}

void fn_8010D3C4(Eft* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_8010D400(self);
        return;
    case 1:
        fn_8010D50C(self);
        return;
    case 2:
        fn_8010D5F4(self);
        return;
    case 3:
        fn_8010D604(self);
        return;
    }
}

void fn_8010D5F4(Eft* self)
{
    self->state_0x05++;
}

void fn_8010D604(Eft* self)
{
    fn_800F886C(self);
}

/* Creates the effect this state runs and orients it by the record's rotation and position. */
#pragma peephole off /* the pool-block store keeps the target's reload - playbook 39 */
void fn_8010D400(Eft* self)
{
    Mtx34 mtx;
    EftWork* work;
    u32 color;

    MTX34_ctor(&mtx);
    work = self->work_0x38;
    self->state_0x05++;
    work->effect = res_eft_create__FUsUsUl(lbl_8059F588[self->type_0x02],
                                           lbl_8059F59C[self->type_0x02], 0);
    if (work->effect == 0) {
        fn_8010D604(self);
        return;
    }
    cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(&self->rot_0x24.vec, &mtx);
    fn_80101428(&mtx, &self->pos_0x18);
    SetRootMtx__Q34nw4r2ef6EffectFRCQ34nw4r4math5MTX34(work->effect, &mtx);
    self->flag_0x01 = 1;
    color = get_stg_eft_col__FUcUc(self->areano_0x44, 0);
    work->color_0x08.c.r = (color & 0xFF000000) >> 24;
    work->color_0x08.c.g = (color & 0x00FF0000) >> 16;
    work->color_0x08.c.b = (color & 0x0000FF00) >> 8;
    work->color_0x08.c.a = 0xFF;
    change_paramscale_eff__FPQ34nw4r2ef6Effectf(work->effect, work->scale_0x0C);
    fn_8010D50C(self);
}
#pragma peephole reset

/* Advances the state while the effect lives, recolouring it from the record's colour. */
void fn_8010D50C(Eft* self)
{
    EftWork* work;
    u32 color;

    work = self->work_0x38;
    if (self->demo_flag_0x08 == 1 && event_demo_ck__Fv() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->areano_0x44 != get_now_areano__Fv()) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (effect_move__FPQ34nw4r2ef6Effect(work->effect) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    color = work->color_0x08.rgba;
    change_color_eff__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC38_GXColor(work->effect, &self->pos_0x18, &color);
    fn_800F93D8(self, &work->effect, 1, work->count, 0);
}

/* Spawns the sub-effect whose variant depends on the current game mode. */
void fn_8010D608(Plw* source)
{
    if (fn_800CF208() == 2) {
        if (fn_8010D70C(source, 4, 0, 1) == 0) {
        }
    } else {
        if (fn_8010D70C(source, 0, 0, 2) == 0) {
        }
    }
}

/* Spawns the third sub-effect variant. */
void fn_8010D678(Plw* source)
{
    fn_8010D70C(source, 3, 2, 1);
}

/* Spawns a sub-effect that follows the player's weapon joint. */
void fn_8010D688(Plw* self)
{
    Eft* eft;
    EftWork* work;

    eft = fn_8010D70C(self, 2, 0, 1);
    if (eft != NULL) {
        work = eft->work_0x38;
        work->field_0x68 = self->field_0x60;
        work->field_0x6C = self->field_0x7C;
        fn_800E0A14(self->physics_0x13C + 4, 7, &work->body_0x34.matrix_0x34);
        fn_800F9DF4(eft, 1, 4);
    }
}

/* Advances the state: a per-variant handler while the state is 0, then the generic ones. */
void fn_8010D8B0(Eft* self)
{
    switch (self->state_0x05) {
    case 1:
        fn_8010E008(self);
        return;
    case 2:
        fn_80111624(self);
        return;
    case 3:
        fn_80111634(self);
        return;
    case 0:
        switch (self->type_0x02) {
        case 0:
            fn_8010D928(self);
            return;
        case 2:
            fn_8010DA48(self);
            return;
        case 3:
            fn_8010DE40(self);
            return;
        case 4:
            fn_8010DF38(self);
            return;
        }
        break;
    }
}

/* Runs the per-variant body of the effect this record was created for. */
void fn_8010E008(Eft* self)
{
    switch (self->type_0x02) {
    case 0:
        fn_8010E2A8(self);
        return;
    case 2:
        fn_80110A18(self);
        return;
    case 3:
        fn_80110FE0(self);
        return;
    case 4:
        fn_80111344(self);
        return;
    }
}

/* Allocates the sub-effect record and its per-variant handles. */
Eft* fn_8010D70C(Plw* source, s32 arg1, s32 arg2, s32 arg3)
{
    Eft* eft;
    EftWork* work;
    s32 i;

    eft = fn_800F8788(0x70);
    if (eft == NULL) {
        return NULL;
    }
    eft->type_0x02 = arg1;
    eft->cb_0x40 = fn_8010D824;
    work = eft->work_0x38;
    work->count = (u8)arg3;
    for (i = 0; i < work->count; i++) {
        work->slots_0x10.items.items_0x10[i] = fn_800F8914();
        if (work->slots_0x10.items.items_0x10[i] == 0) {
            fn_800F886C(eft);
            return NULL;
        }
    }
    work->field_0x67 = 0;
    eft->flag_0x01 = 1;
    eft->kind_0x03 = 0x12;
    eft->field_0x0C = 0;
    fn_800F9DF4(eft, 1, 0);
    switch ((u8)arg2) {
    case 0:
        eft->source_0x30 = source;
        break;
    case 2:
        eft->source_0x30 = source;
        break;
    }
    eft->cb_0x34 = fn_8010D8B0;
    return eft;
}
#pragma peephole reset

/* Releases the sub-effect's handles and clears its count. */
void fn_8010D824(Eft* self)
{
    EftWork* work;
    s32 i;

    work = self->work_0x38;
    fn_800F8A44(&work->slots_0x10.items.items_0x10[0], work->count);
    for (i = 0; i < 5; i++) {
        if (work->slots_0x10.handles.handles_0x18[i] != 0) {
            push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl(&work->slots_0x10.handles.handles_0x18[i], 1);
        }
    }
    work->count = 0;
}

/* Advances the state. */
void fn_80111624(Eft* self)
{
    self->state_0x05++;
}

/* Drops the sub-effect record. */
void fn_80111634(Eft* self)
{
    fn_800F886C(self);
}

/* Shows only the numbered light of the first item. */
void fn_80111638(Eft* self, u32 arg1)
{
    EftWork* work;
    u32 i;

    work = self->work_0x38;
    for (i = 1; i <= 0x13; i++) {
        if (i == arg1) {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 1);
        } else {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 0);
        }
    }
}

/* Shows one or all of the second item's lights. */
void fn_801116B0(Eft* self, u32 arg1)
{
    EftWork* work;
    s32 i;

    work = self->work_0x38;
    if (arg1 == 2) {
        for (i = 1; i < 3; i++) {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[1], i, 1);
        }
        return;
    }
    for (i = 1; i < 5; i++) {
        if (i == arg1) {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[1], i, 1);
        } else {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[1], i, 0);
        }
    }
}

/* Shows one or all of the first item's lights. */
void fn_80111754(Eft* self, u32 arg1)
{
    EftWork* work;
    s32 i;

    work = self->work_0x38;
    if (arg1 == 2) {
        for (i = 1; i < 3; i++) {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 1);
        }
        return;
    }
    for (i = 1; i < 5; i++) {
        if (i == arg1) {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 1);
        } else {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 0);
        }
    }
}

/* Shows only the numbered light of the first item. */
void fn_801117F8(Eft* self, u32 arg1)
{
    EftWork* work;
    s32 i;

    work = self->work_0x38;
    for (i = 1; i <= 3; i++) {
        if (i == arg1) {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 1);
        } else {
            setVisibility__6MHcharFUlb(work->slots_0x10.items.items_0x10[0], i, 0);
        }
    }
}

/* Creates the light per item and seeds the day-cycle state. */
void fn_8010D928(Eft* self)
{
    Plw* source;
    EftWork* work;
    EftLight** item;
    u16* id;
    s32 i;
    s32 joint;

    source = self->source_0x30;
    work = self->work_0x38;
    self->state_0x05++;
    joint = source->field_0x08 + 3;
    item = &work->slots_0x10.items.items_0x10[0];
    id = lbl_80791838;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create_light__FP6MHcharUsUll(*item, *id, 0x12C, joint) == 0) {
            fn_80111634(self);
            return;
        }
        id++;
        item++;
    }
    fn_80111638(self, 0);
    fn_801116B0(self, 0);
    for (i = 0; i < work->count; i++) {
        setVector3__FPQ34nw4r4math4VEC3fff(&work->slots_0x10.items.items_0x10[i]->pos_0x1C, lbl_80796848, lbl_80796848, lbl_80796848);
        copyVec3(&work->effect, &work->slots_0x10.items.items_0x10[i]->pos_0x1C);
        work->slots_0x10.items.items_0x10[i]->field_0x35 = 0;
    }
    work->field_0x2C = fn_802BF814();
    work->field_0x2E = source->field_0x16;
    fn_8010E008(self);
}

/* Creates one light per item at the fixed joint and seeds the vectors. */
void fn_8010DE40(Eft* self)
{
    EftWork* work;
    s32 i;

    work = self->work_0x38;
    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create_light__FP6MHcharUsUll(work->slots_0x10.items.items_0x10[i], 4, 0x2C, 6) == 0) {
            fn_80111634(self);
            return;
        }
    }
    fn_80111754(self, 0);
    for (i = 0; i < work->count; i++) {
        setVector3__FPQ34nw4r4math4VEC3fff(&work->slots_0x10.items.items_0x10[i]->pos_0x1C, lbl_80796848, lbl_80796848, lbl_80796848);
        copyVec3(&work->effect, &work->slots_0x10.items.items_0x10[i]->pos_0x1C);
        work->slots_0x10.items.items_0x10[i]->field_0x35 = 0;
    }
    work->field_0x2C = fn_802BF814();
    fn_8010E008(self);
}

/* Creates the single light at the named joint and seeds the vectors. */
void fn_8010DF38(Eft* self)
{
    EftWork* work;
    s32 i;

    work = self->work_0x38;
    self->state_0x05++;
    if (res_eft_model_create_light__FP6MHcharUsUll(work->slots_0x10.items.items_0x10[0], 0xA7, 0x12C, 0) == 0) {
        fn_80111634(self);
        return;
    }
    fn_801117F8(self, 0);
    for (i = 0; i < work->count; i++) {
        setVector3__FPQ34nw4r4math4VEC3fff(&work->slots_0x10.items.items_0x10[i]->pos_0x1C, lbl_80796848, lbl_80796848, lbl_80796848);
        copyVec3(&work->effect, &work->slots_0x10.items.items_0x10[i]->pos_0x1C);
        work->slots_0x10.items.items_0x10[i]->field_0x35 = 0;
    }
    fn_8010E008(self);
}

void fn_80111CD4(u8 arg0, s32 arg1, Rgb3* arg2, Rgb3* arg3, Rgb3* arg4, Rgb3* arg5)
{
    switch (arg0) {
    case 0:
        arg2->r = lbl_8059F5B0[0].r;
        arg2->g = lbl_8059F5B0[0].g;
        arg2->b = lbl_8059F5B0[0].b;
        arg3->r = lbl_8059F5C8[0].r;
        arg3->g = lbl_8059F5C8[0].g;
        arg3->b = lbl_8059F5C8[0].b;
        arg4->r = lbl_8059F5B0[(u8)arg1].r;
        arg4->g = lbl_8059F5B0[(u8)arg1].g;
        arg4->b = lbl_8059F5B0[(u8)arg1].b;
        arg5->r = lbl_8059F5C8[(u8)arg1].r;
        arg5->g = lbl_8059F5C8[(u8)arg1].g;
        arg5->b = lbl_8059F5C8[(u8)arg1].b;
        return;
    case 1:
        arg4->r = lbl_8059F5B0[0].r;
        arg4->g = lbl_8059F5B0[0].g;
        arg4->b = lbl_8059F5B0[0].b;
        arg5->r = lbl_8059F5C8[0].r;
        arg5->g = lbl_8059F5C8[0].g;
        arg5->b = lbl_8059F5C8[0].b;
        arg2->r = lbl_8059F5B0[(u8)arg1].r;
        arg2->g = lbl_8059F5B0[(u8)arg1].g;
        arg2->b = lbl_8059F5B0[(u8)arg1].b;
        arg3->r = lbl_8059F5C8[(u8)arg1].r;
        arg3->g = lbl_8059F5C8[(u8)arg1].g;
        arg3->b = lbl_8059F5C8[(u8)arg1].b;
        return;
    }
}

/* Picks the frame-range index for the player's current motion. */
s32 fn_80111DFC(Plw* self)
{
    switch (Get_motion_no__FP4_PLW(self)) {
    case 0x154:
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_80796910, lbl_80796884) != 0) {
            return 7;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_8079688C, lbl_80796884) != 0) {
            return 8;
        }
        return 9;
    case 0x155:
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_80796910, lbl_80796884) != 0) {
            return 8;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_807968B0, lbl_80796884) != 0) {
            return 9;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_8079690C, lbl_80796884) != 0) {
            return 8;
        }
        return 7;
    case 0x156:
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_807969D0, lbl_80796884) != 0) {
            return 8;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_80796934, lbl_80796884) != 0) {
            return 9;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_8079692C, lbl_80796884) != 0) {
            return 8;
        }
        return 7;
    case 0x157:
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_80796910, lbl_80796884) != 0) {
            return 8;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_8079689C, lbl_80796884) != 0) {
            return 9;
        }
        if (Pl_frame_check__FP4_PLWUlff(self, 2, lbl_807969D4, lbl_80796884) != 0) {
            return 8;
        }
        return 7;
    default:
        return 7;
    }
}

/* Attaches a light effect to the model for the named motion. */
void fn_80111BC0(Eft* self, EftModel* model, u16 arg2)
{
    EftWork* work;
    void* p;

    work = self->work_0x38;
    if ((arg2 - 0x15D) <= 1 || (arg2 == 0x150 && fn_8026A328(1, lbl_80796884, lbl_80796884) != 0)) {
        if (self->areano_0x44 != get_now_areano__Fv() || self->flag_0x01 == 0) {
            fn_8007F0CC(pRoot, model->field_0x118);
            return;
        }
        if (model != NULL && model->field_0x118 == model->field_0x10C->field_0x4) {
            p = fn_800E3B8C(1, model->field_0x35, 0, 0x40, 0, fn_80111870);
            if (p != NULL) {
                ((EftLightObj*)p)->owner_0x2C = self;
            }
            fn_8007F0CC(pRoot, model->field_0x118);
        }
    } else {
        fn_800F93D8(self, &work->slots_0x10.items.items_0x10[0], 2, 1, 0);
    }
}
