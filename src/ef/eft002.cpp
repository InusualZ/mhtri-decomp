/* ef/eft002.cpp - the tail of effect 001 and effect 002 (`eft001_set_pos`, `eft002_set`, `eft002_set_shell`)
 *
 * `.text` 0x800FBE64..0x800FD520, 24 functions written (the rest of the range is not decompiled yet).
 * Phase 4: fold of 2 registered units, built from `ef/eft001.cpp`, `ef/eft002.cpp`.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 *
 * Kept views: the retired sources declared 6 callee(s) with different signatures (`eft_res_slot_get`, `eft_res_models_spawn`, `eft_state_flags_set`, `fn_800FCED4`, `fn_800FD29C`, `fn_800FD2AC`); each function keeps its own source's view through a function-pointer cast macro (`<name>_viewN`, `<name>_cN`), which compiles to the same direct call, so the fold does not move any body.
 * Hidden declarations: 3 header declaration(s) that disagree with the kept view are renamed away around their `#include` (`#define <name> <name>_hidden_<header>`): `fn_800FCED4`, `fn_800FD29C`, `fn_800FD2AC`.
 */

/* Retired header of `ef/eft002.cpp` (kept for its notes and residuals): */
/* auto/800FCED4_fn_800FCED4.cpp - the `eft002` effect cluster, 0x800FCED4..0x800FD520 (8 functions).
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * Two effect families share the range. `eft002_set` / `eft002_set_shell` (0x800FD2FC, 0x800FD3D8) build
 * the 72-byte effect object `eft_res_slot_release` destroys and install the two handlers that travel with it
 * (`fn_800FD4E4` = the `byte_5` dispatcher, `fn_800FD4A8` = the pool release). `fn_800FCED4` /
 * `fn_800FD29C` / `fn_800FD2AC` / `fn_800FD2B0` are the other family: the per-frame body, its `byte_5`
 * bump, the destructor thunk (`fn_800FD2AC` -> `eft_res_slot_release`, called from four sites in the unit before
 * this one) and the "is this effect still legal for the player" gate.
 *
 * The seam is the `.sdata2` pool jump `lbl_80796684 -> lbl_80796688`; the reasoning is in configure.py
 * beside the `auto` lib entry. Every `fn_XXXXXXXX` callee and the SDK entry points are `extern "C"` so
 * the compiler emits the map's spelling; the mangled map names (`eft002_set__FP4_PLWUcUcf`,
 * `getKeyData__FPff`, `setMatColor__6MHcharFUl12_GXChannelID8_GXColorb`, ...) come from C++ declarations
 * with the signatures the map encodes.
 *
 * Result: all 8 symbols 100 %, unit 100 % fuzzy, `.text` (0x64C), `extab` (0x20) and `extabindex` (0x30)
 * byte-identical to the target. The one flag-shaped lever is the peephole pass - see the scoped
 * `#pragma peephole off`/`reset` pair below the `extern` block. The only remaining object-level difference
 * is two relocation *names*: our object
 * emits the `fn_800FD2B0` jump table and MWCC's int->float magic as local pool entries where the target
 * (whose splits.txt does not claim them) references `jumptable_8059B930` and `lbl_80796650`.
 *
 * Load-bearing source shapes (each one measured, the wrong form costs real points):
 *   * the two returned values that are consumed by pointer (`fn_8006F304(&srt, access.GetResTexSrt(false))`,
 *     `fn_800532DC(&mtx_a, get_current_view_mtx())`) are passed through a **reference** parameter, so MWCC
 *     materialises the return as a compiler temporary below the named locals. With a named `u32 res` local
 *     the two swap slots (res 0x18 / srt 0x14 against retail's srt 0x1c / res 0x14); with a named
 *     `MTX34 view_mtx = get_current_view_mtx()` MWCC emits a 48-byte copy and the frame grows
 *     0x140 -> 0x170 (86.11 % -> 99.48 %).
 *   * `fn_800FD2B0`'s switch must cover case 0, so MWCC's table is 0-based (`cmplwi r0,29`, no subtract) and
 *     the `flag != 1` return lands *after* the switch; `_PLW::flag_0x30` is `s8` (retail `cmpwi`, not
 *     `cmplwi`).
 *   * `fn_800FCED4`'s state machine is a `switch (state_0x06)` whose inner type switch uses `break` plus a
 *     trailing `return` - with `return` in each case MWCC adds a dead `b`. `state_0x06++` (not `= 1`) so
 *     the increment reuses the switch operand. `frame_0x10` is `s32`: retail's float conversion is
 *     `xoris` + `2^52+2^31`, which MWCC only picks for a signed source.
 *
 * Data: the unit's `.data` run (0x8059B6F8..0x8059B9A8: the RGB key tables, the two `getKeyData` float
 * tables, the `_GXColor` alpha tables, `jumptable_8059B930`) and its `.sdata`/`.sdata2` pool
 * (`lbl_807916D0`/`D8`, `lbl_80796650`) are `extern` here and referenced by name (playbook 23/29) - the
 * target object carries no such sections, and a range our object does not emit must not be claimed. Two of
 * them the object *does* emit and they are the `config_requests`: the `fn_800FD2B0` jump table
 * (`.data 0x78`) and the int->float magic (`.sdata2 0x8`). Making the unit linkable needs the rest of the
 * run defined in the source and claimed in one measured data pass; the bytes are in
 * `.pi/notes/800fced4-fn-800fced4-063a.md`.
 *
 * The pool block at +0x38 is typed per effect family: the setters seed a `f32` scale and a `u32` id
 * (`_EFT_WORK`), `fn_800FCED4` reads an `MHchar*` model and an `f32` paramscale out of the same words
 * (`_EFT_MODEL_WORK`) - a float and a pointer in one slot cannot be the same object, so the two 72-byte
 * effect objects are two types here rather than one with a punning field.
 *
 * `_PLW`, `_SHELL_W`, `MHchar`, `_GXChannelID`/`_GXColor` and the `nw4r` types are reconstructed minimally
 * (only the offsets/sizes this unit needs) and are shared with other units - they belong in one header,
 * which does not exist yet.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x800f0000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-4.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-4.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800FCED4_fn_800FCED4.cpp`.
 * The name is provisional - `auto/` plus the first symbol's address.
 */

#include "ef/eft_res_spawn_gate_ck.h" /* eft_res_spawn_gate_ck (rule 2: the owner's header) */
#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "ef/eft_res_model_get.h" /* eft_res_model_get (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "enemy/ENEMY_WORK.h"
#include "sound/fn_800D7F54.h"
#define fn_800FCED4 fn_800FCED4_hidden_eft002_h
#define fn_800FD29C fn_800FD29C_hidden_eft002_h
#define fn_800FD2AC fn_800FD2AC_hidden_eft002_h
#include "ef/eft002.h"
#undef fn_800FD2AC
#undef fn_800FD29C
#undef fn_800FCED4
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */
#include "ef/fn_800FD520.h"
#include "ef/fn_800FD718.h"
#include "unsplit/g3d.h"
#include "g3d/g3d_state.h"
#include "unsplit/sound.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(void*, u8, u8))eft_state_flags_set)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800F8914_c1 ((void* (*)())eft_res_model_get)
/* fn_800FD2AC_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800FD2AC_view1 ((void (*)(struct _EFT*))fn_800FD2AC)
/* fn_800FD29C_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800FD29C_view1 ((void (*)(struct _EFT*))fn_800FD29C)
/* fn_800FCED4_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800FCED4_view1 ((void (*)(struct _EFT*))fn_800FCED4)
/* fn_800F9DF4_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800F9DF4_view1 ((void (*)(_EFT*, u8, u8))eft_state_flags_set)
/* fn_800F93D8_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800F93D8_view1 ((void (*)(void*, void*, u32, s32, u32))eft_res_models_spawn)
/* fn_800F8788_view1: the retired sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800F8788_view1 ((_EFT* (*)(u32))eft_res_slot_get)

/* The `fn_800FBE64` / `fn_800FC484` family block: two pooled effects and the model they share. */
struct _EFT001_EFFECT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effect_0x04;  /* the primary pooled effect */
    /* +0x08 */ nw4r::ef::Effect* effect_0x08;  /* the secondary effect / the joint model (cast per family) */
    /* +0x0C */ f32 scale_0x0C;
    /* +0x10 */ u8 unused_0x10[4];
    /* +0x14 */ u8 color_0x14[4];
    /* +0x18 */ union { f32 value_0x18; s8 timer_0x18; };
    /* +0x1C */ u8 unused_0x1C[0x0C];
    /* +0x28 */ u8 flag_0x28;
    /* +0x29 */ u8 flag_0x29;
    /* +0x2A */ u8 unused_0x2A[2];
    /* +0x2C */ s32 value_0x2C;
    /* +0x30 */ s32 value_0x30;
};
/* size: 0x34 - lower bound, an approximation. */

/* The opaque source record the `fn_800FBE64` / `fn_800FC1DC` builders read: the area byte at +0x0F
 * and the joint/parameter word at +0x36.  Only the offsets this unit reads are named.
 * size: 0x3C - lower bound, an approximation. */
struct _EFT001_SRC {
    /* +0x00 */ u8 unused_0x00[0x0F];
    /* +0x0F */ u8 area_0x0F;      /* the area the builder is gated on */
    /* +0x10 */ u8 unused_0x10[0x26];
    /* +0x36 */ u16 param_0x36;     /* the joint/parameter passed to the model builder */
    /* +0x38 */ u8 unused_0x38[0x04];
};

/* One particle-manager entry `fn_800FCEC8` walks: the manager sits after an 88-byte header.
 * size: 0x59 - lower bound. */
struct _EFT001_PM_ENTRY {
    /* +0x00 */ u8 unused_0x00[0x58];
    /* +0x58 */ u8 field_0x58;
};

/* ---------------------------------------------------------------------------------------------------
 * the mangled callees (real C++ declarations - rule 9: the member call, never the mangled spelling)
 * ------------------------------------------------------------------------------------------------- */
u32 get_now_areano();
s32 ran_suu(s32 range);
/* untyped: an opaque handle passed through (the model record is only stored) */
void* res_eft_model_create(MHchar* model, u16 id, u32 arg);
nw4r::ef::Effect* res_eft_create(u16 id, u16 param, u32 idx);
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
void change_paramscale_eff_vec3(nw4r::ef::Effect* effect, nw4r::math::VEC3* vec);
u32 effect_move(nw4r::ef::Effect* effect);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);

void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);

extern "C" {
void fn_800834F0(void* p);

void fn_800DB6CC(nw4r::math::VEC3* pos);
void fn_800DB714(nw4r::math::VEC3* pos);
void fn_800DB75C(void* enemy, nw4r::math::VEC3* pos);

u32 fn_8029F564(void* self, u32 flag);

/* untyped: an opaque handle passed through (the pooled slot is only stored) */
void* eft_res_slot_get(u32 pool);
void eft_res_slot_release(void* self);
void fn_800F8A44(void* p, s32 mode);

void eft_res_models_spawn(void* self, nw4r::ef::Effect** effects, s32 count, s32 mode, void* arg);
u8 fn_800F9D80(void* self);
/* untyped: an opaque handle passed through (the effect record is only handed on) */

void fn_800AA75C(void* dst);

extern u16 lbl_8059B638[];
extern u16 lbl_8059B670[];

extern f32 lbl_80796644;
extern f32 lbl_80796640;
extern f32 lbl_80796648;
extern u8 lbl_8059B6A8[];
extern u8 lbl_8059B6B4[];
extern u8 lbl_8059B6CC[];
extern u8 lbl_8059B6E4[];
extern u8 lbl_8059B7C8[];
}

extern "C" void fn_800FC3A4(_EFT* self);
extern "C" void fn_800FC3E0(_EFT* self);
extern "C" void fn_800FC484(_EFT* self);
extern "C" void fn_800FC7EC(_EFT* self);
extern "C" void fn_800FCA34(_EFT* self);
extern "C" void fn_800FCA54(_EFT* self);



namespace nw4r {

namespace ef {
struct Effect;
}

namespace g3d {
/* The texture-SRT handle the material access wraps: 4 bytes, returned and passed by value. */
struct ResTexSrt { /* size: 0x04 */
    /* +0x00 */ u32 handle;

    void GetEffectMtx(u32 idx, nw4r::math::MTX34* out) const;
    void SetEffectMtx(u32 idx, const nw4r::math::MTX34* in);
};

/* The 0x34-byte material-access block the code constructs on the stack. `ScnMdl` itself is opaque
 * here (only ever used through pointers), so its size is a lower bound. */
struct ScnMdl { /* size: 0x04 - lower bound, an approximation (opaque here) */
    struct CopiedMatAccess { /* size: 0x34 */
        /* +0x00 */ u32 handle;
        /* +0x04 */ u8 unused_0x04[0x34 - 0x04];

        CopiedMatAccess(ScnMdl* mdl, u32 idx);
        u32 GetResTexSrt(bool keep); /* the handle, returned in r3 */
    };
};
}
}



/* ---------------------------------------------------------------------------------------------------
 * the game work types the setters take
 * ------------------------------------------------------------------------------------------------- */

/* The item/shell the player has equipped, reached through `_PLW::equip_0x2C`. Only its type byte at
 * +0x03 (0..29, the index `fn_800FD2B0` gates on) is read here.
 * size: 0x0C - lower bound, an approximation. */
struct _SHELL_W {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 type_0x03;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8 area_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
};





/* size: 0x48 */

/* Pool block of the `fn_800FCED4` family: the state machine drives a model and its parameter scale. */
struct _EFT_MODEL_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effect;
    /* +0x08 */ MHchar* model;
    /* +0x0C */ f32 paramscale;
};
/* size: 0x10 - lower bound, an approximation. */

/* The effect object `fn_800FCED4` drives: the same 72-byte layout, with the model pool block at +0x38. */
struct _EFT_MODEL {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 state_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 unused_0x08[0x0C - 0x08];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 frame_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ u8 unused_0x24[0x38 - 0x24];
    /* +0x38 */ _EFT_MODEL_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x48 - 0x3C];
};
/* size: 0x48 */

/* ---------------------------------------------------------------------------------------------------
 * externs
 * ------------------------------------------------------------------------------------------------- */
extern "C" void mtx34_identity(void* mtx);
extern "C" void fn_800532DC(void* dst, const nw4r::math::MTX34& src);
extern "C" void mtx34_concat_assign(void* dst, void* src);
extern "C" void fn_800FBB90(void* mtx, nw4r::math::VEC3* pos);
extern "C" void eft_res_slot_release(void* self);
extern "C" void fn_800FD4A8(_EFT* self);
extern "C" void fn_800FD4E4(_EFT* self);

nw4r::math::MTX34 get_current_view_mtx();
f32 getKeyData(f32* keys, f32 frame);

u8 eftGetKeyAlpha(u8* keys, long frame);

/* The unit's own `.data`/`.sdata`/`.sdata2` run, referenced but not emitted (see the header). */
extern "C" u8 lbl_8059B6F8[0x10];
extern "C" u8 lbl_8059B708[0x14];
extern "C" u8 lbl_8059B71C[0x14];
extern "C" u8 lbl_8059B730[0x14];
extern "C" u8 lbl_8059B744[0x0C];
extern "C" u8 lbl_8059B750[0x10];
extern "C" f32 lbl_8059B760[6];
extern "C" f32 lbl_8059B778[8];
extern "C" u8 lbl_807916D0[8];
extern "C" u8 lbl_807916D8[8];

extern "C" void fn_800FD2AC(void* self);
extern "C" void fn_800FD29C(_EFT_MODEL* self);
extern "C" void fn_800FCED4(_EFT_MODEL* self);

/* Copies a 12-byte rotation triple. */
extern "C" void eft_rot_vec_copy(_CP_VECTOR* dst, _CP_VECTOR* src)
{
    *dst = *src;
}

/* Dispatches to the two per-frame model handlers by the effect type. */
extern "C" void fn_800FC384(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s8)self->type_0x02 == 14) {
        fn_800FC3E0(self);
    } else {
        fn_800FC3A4(self);
    }
}

/* Retires the pooled effect of the work block and clears its count. */
extern "C" void fn_800FC3A4(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;

    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->effect_0x04, work->count);
    work->count = 0;
}

/* Releases the secondary effect's pool entry, retires the primary and clears the count. */
extern "C" void fn_800FC3E0(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;

    fn_800F8A44(&work->effect_0x08, 1);
    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->effect_0x04, work->count);
    work->count = 0;
}

/* The per-frame dispatcher: advance the state handler the effect's `byte_5` selects. */
extern "C" void fn_800FC428(_EFT* self)
{
    switch (self->state_0x05) {
    case 1:
        fn_800FCA34(self);
        break;
    case 2:
        fn_800FD29C_view1(self);
        break;
    case 3:
        fn_800FD2AC_view1(self);
        break;
    case 0:
        if ((u32)(self->type_0x02 - 18) <= 1 || (s8)self->type_0x02 == 14) {
            fn_800FC7EC(self);
        } else {
            fn_800FC484(self);
        }
        break;
    }
}

/* Dispatches to the two per-frame model handlers of the second half of the unit. */
extern "C" void fn_800FCA34(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s8)self->type_0x02 == 14) {
        fn_800FCED4_view1(self);
    } else {
        fn_800FCA54(self);
    }
}

/* Builds a model effect at `pos`: gated on the current area, allocates a 16-entry pool effect,
 * installs the two per-frame handlers, seeds the model list and the parameter scale. */
extern "C" _EFT* fn_800FC27C(nw4r::math::VEC3* pos, u32 type, u32 param, u32 area, f32 scale)
{
    if ((u8)area != (u8)get_now_areano()) {
        return NULL;
    }

    _EFT* effect = (_EFT*)eft_res_slot_get(16);
    if (effect == NULL) {
        return NULL;
    }

    effect->type_0x02 = (u8)type;
    effect->release_0x40 = fn_800FC384;
    effect->dispatch_0x34 = fn_800FC428;

    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)effect->work_0x38;
    work->count = 1;
    work->effect_0x08 = (nw4r::ef::Effect*)fn_800F8914_c1();
    if (work->effect_0x08 == NULL) {
        eft_res_slot_release(effect);
        return NULL;
    }

    work->scale_0x0C = scale;
    effect->field_0x03 = 1;
    effect->timer_0x0C = 0;
    copyVec3(&effect->pos_0x18, pos);
    effect->rot_0x24.y = param;
    effect->area_0x44 = (u8)area;
    eft_state_flags_set_c1(effect, 0, 4);
    return effect;
}

/* Wrapper around `fn_800FC27C` for a `_CP_VECTOR` rotation triple: its second word is the parameter
 * stored on the effect. */
void eft001_set_pos(nw4r::math::VEC3* pos, u32 a, _CP_VECTOR* v, u8 b, f32 c)
{
    if (fn_800FC27C(pos, a, v->y, b, c) == NULL) {
        return;
    }
}

/* Builds a model effect from a joint source: its +0x36 word and +0x0F area seed `fn_800FC27C`, and the
 * effect's `field_0x07` records whether the source owns the 0x800 flag. */
extern "C" void fn_800FC1DC(nw4r::math::VEC3* pos, u32 type, _EFT001_SRC* src)
{
    _EFT* effect = fn_800FC27C(pos, type, src->param_0x36, src->area_0x0F, lbl_80796644);

    if (effect != NULL) {
        effect->field_0x07 = (fn_8029F564(src, 2048) == 1) ? 1 : 0;
    }
}

/* Builds a model effect with a full parameter set: the area gate, the model builder, the effect's
 * `field_0x08`/`field_0x06` seed bytes, the rotation triple and the parameter scale. */
extern "C" void fn_800FC0F0(nw4r::math::VEC3* pos, u32 type, u32 field_08, u32 area, _CP_VECTOR* rot, f32 scale)
{
    if ((u8)area != (u8)get_now_areano()) {
        return;
    }

    _EFT* effect = (_EFT*)eft_res_slot_get(44);
    if (effect == NULL) {
        return;
    }

    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)effect->work_0x38;
    work->count = 1;
    effect->type_0x02 = (u8)type;
    effect->demo_flag_0x08 = (u8)field_08;
    effect->field_0x03 = 1;
    effect->timer_0x0C = 0;
    copyVec3(&effect->pos_0x18, pos);
    eft_rot_vec_copy(&effect->rot_0x24, rot);
    effect->area_0x44 = (u8)area;
    work->value_0x18 = scale;
    effect->field_0x07 = 4;
    effect->field_0x06 = 0;
    eft_state_flags_set_c1(effect, 0, 4);
    effect->release_0x40 = fn_800FC384;
    effect->dispatch_0x34 = fn_800FC428;
}

/* The particle-manager walk callback: forwards the entry (offset by its 88-byte header) to the shared
 * particle release. */
extern "C" void fn_800FCEC8(void* entry, u32 index)
{
    (void)index;
    fn_800AA75C(&((_EFT001_PM_ENTRY*)entry)->field_0x58);
}

/* Walks the effect's particle-manager pool through `ForeachParticleManager`. */
extern "C" void fn_800FCEB0(nw4r::ef::Effect* self, u32 arg, bool flag)
{
    self->ForeachParticleManager(fn_800FCEC8, arg, flag);
}

/* Builds the pooled effect + model pair for a per-frame effect: allocates both from the type's id
 * tables, poses the model at the effect's position, offsets its animation clock by a random amount,
 * and seeds the two K-colours the effect family uses. */
extern "C" void fn_800FC7EC(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;
    MHchar* model;
    _GXColor color;

    self->state_0x05++;

    work->effect_0x04 = res_eft_create(lbl_8059B638[self->type_0x02], lbl_8059B670[self->type_0x02], 0);
    if (work->effect_0x04 == NULL) {
        fn_800FD2AC_view1(self);
        return;
    }

    if (res_eft_model_create((MHchar*)work->effect_0x08, 82, 340) == NULL) {
        fn_800FD2AC_view1(self);
        return;
    }

    model = (MHchar*)work->effect_0x08;
    SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
    copyVec3(&model->pos_0x04, &self->pos_0x18);
    eft_rot_vec_copy(&model->rot_0x54, &self->rot_0x24);

    model->field_0x2C -= 3641;
    model->field_0x2C += ran_suu(0) & 0x1FFF;
    model->field_0x30 = 0xF1C7;
    model->field_0x30 += ran_suu(0) & 0x1FFF;

    model->setVisibility(1, false);
    setVector3(&model->scale_0x1C, work->scale_0x0C, work->scale_0x0C, work->scale_0x0C);

    self->timer_0x0C = 5;
    self->flag_0x01 = 1;

    switch (self->type_0x02) {
    case 14:
        color.r = 0x66;
        color.g = 0xEE;
        color.b = 0xFF;
        color.a = 0xFF;
        break;
    case 18:
        color.r = 0xFF;
        color.g = 0;
        color.b = 0;
        color.a = 0xFF;
        {
            s32 i;
            for (i = 0; i < 2; i++) {
                model->setMatAlphaBlendMode(i, GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            }
        }
        break;
    case 19:
        color.r = 0xFF;
        color.g = 0xFF;
        color.b = 0xC8;
        color.a = 0xFF;
        break;
    }

    {
        s32 i;
        for (i = 0; i < 2; i++) {
            model->setTevKColor(i, GX_KCOLOR3, &color);
        }
    }

    self->field_0x06 = 0;
    fn_800FCA34(self);
}

/* The per-frame body of the non-model (type 11..26) effect: advances the two-stage machine, moves
 * the pooled effects, and recolours them per type from the key tables. */
extern "C" void fn_800FCA54(_EFT* self)
{
    u8 buf[8];
    nw4r::math::VEC3 vec;
    _GXColor color;
    s32 live = 0;
    s32 i;
    _EFT001_EFFECT_WORK* work;

    fn_800834F0(buf);
    VEC3_ctor(&vec);
    work = (_EFT001_EFFECT_WORK*)self->work_0x38;

    if (self->field_0x06 == 0) {
        if (--self->timer_0x0C > 0) {
            return;
        }

        self->field_0x06++;

        switch (self->type_0x02) {
        case 7:
        case 8:
            fn_800DB608(2, &self->pos_0x18, work->flag_0x29);
            break;
        case 15:
            fn_800DB608(3, &self->pos_0x18, work->flag_0x29);
            break;
        case 17:
            fn_800DB608(4, &self->pos_0x18, work->flag_0x29);
            break;
        case 16:
            fn_800DB608(5, &self->pos_0x18, work->flag_0x29);
            break;
        case 20:
            fn_800DB608(6, &self->pos_0x18, work->flag_0x29);
            break;
        case 21:
            fn_800DB608(7, &self->pos_0x18, work->flag_0x29);
            break;
        case 11:
            switch (self->demo_flag_0x08) {
            case 0:
            case 2:
                fn_800DB6CC(&self->pos_0x18);
                break;
            case 1:
                fn_800DB714(&self->pos_0x18);
                break;
            }
            break;
        case 12:
            if (eft_res_spawn_gate_ck(self, 0) == 1) {
                fn_800DB75C(self->source_0x30, &self->pos_0x18);
            }
            break;
        }
        return;
    }

    if (self->type_0x02 == 7) {
        ((nw4r::math::VEC3*)buf)->x = lbl_80796640;
        ((nw4r::math::VEC3*)buf)->y = lbl_80796640;
        fn_800FCEB0(work->effect_0x04, (u32)buf, true);
    }

    if (self->type_0x02 <= 5 || (u8)(self->type_0x02 - 9) <= 1 ||
        self->type_0x02 == 22 || self->type_0x02 == 26) {
        change_paramscale_eff(work->effect_0x04, work->scale_0x0C);
    } else if (self->type_0x02 == 25) {
        setVector3(&vec, work->scale_0x0C, work->scale_0x0C, lbl_80796648 * work->scale_0x0C);
        change_paramscale_eff_vec3(work->effect_0x04, &vec);
    }

    for (i = 0; i < work->count; i++) {
        if (effect_move(((nw4r::ef::Effect**)&work->effect_0x04)[i]) == 0) {
            live = (u8)(live + 1);
        }
    }

    if (live >= work->count) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    self->field_0x10++;

    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2:
    case 24:
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        break;
    case 7:
    case 8:
        if (fn_800F9D80(self) == 1) {
            work->color_0x14[0] = 100;
            work->color_0x14[1] = 191;
            work->color_0x14[2] = 255;
            work->color_0x14[3] = 255;
            color = *(_GXColor*)work->color_0x14;
            change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        }
        break;
    case 11:
        if (work->flag_0x28 == 0) {
            eftGetKeyRGB(lbl_8059B6E4 + self->demo_flag_0x08 * 4, self->field_0x10, &work->color_0x14[0],
                         &work->color_0x14[1], &work->color_0x14[2]);
        } else {
            eftGetKeyRGB(lbl_8059B6B4, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                         &work->color_0x14[2]);
        }
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        break;
    case 20:
        if (work->flag_0x28 == 0) {
            eftGetKeyRGB(lbl_8059B6A8, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                         &work->color_0x14[2]);
        } else {
            color.r = 255;
            color.g = 175;
            color.b = 175;
            color.a = 255;
            change_color_eff(work->effect_0x04, &self->pos_0x18, color);
            eftGetKeyRGB(lbl_8059B6B4, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                         &work->color_0x14[2]);
        }
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x08, &self->pos_0x18, color);
        break;
    case 21:
        eftGetKeyRGB(lbl_8059B6CC, self->field_0x10, &work->color_0x14[0], &work->color_0x14[1],
                     &work->color_0x14[2]);
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x08, &self->pos_0x18, color);
        break;
    case 25:
    case 26:
        eftGetKeyRGB(lbl_8059B7C8 + self->demo_flag_0x08 * 4, self->field_0x10, &work->color_0x14[0],
                     &work->color_0x14[1], &work->color_0x14[2]);
        color = *(_GXColor*)work->color_0x14;
        change_color_eff(work->effect_0x04, &self->pos_0x18, color);
        break;
    }

    eft_res_models_spawn(self, (nw4r::ef::Effect**)&work->effect_0x04, 1, work->count, NULL);
}

extern "C" void fn_800FC484(_EFT* self) {}
extern "C" void fn_800FBE64(void) {}

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Destroys an effect object: runs the pool release handler, hands the pool block back, then clears the
 * object. */
extern "C" void fn_800FD2AC(void* self)
{
    eft_res_slot_release(self);
}

/* Bumps the effect's `byte_5` state index. */
extern "C" void fn_800FD29C(_EFT_MODEL* self)
{
    self->state_0x05++;
}

/* Answers whether the player's equipped shell type is one of the 22 the effect family supports: 1 for
 * those, 0 for everything else (and for a player without a shell loaded). */
extern "C" u32 fn_800FD2B0(_PLW* self)
{
    if (self->flag_0x30 == 1) {
        switch (self->equip_0x2C->type_0x03) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 12:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 21:
        case 23:
        case 24:
        case 27:
        case 28:
        case 29:
            return 1;
        default:
            return 0;
        }
    }
    return 0;
}

#pragma peephole off

/* Per-frame body of the effect: advances the two-step state machine, drives the texture-SRT animation on
 * both materials, recolours them from the key tables, and re-places the model. */
extern "C" void fn_800FCED4(_EFT_MODEL* self)
{
    nw4r::math::MTX34 mtx_a;
    nw4r::math::MTX34 mtx_b;
    _GXColor color;

    MTX34_ctor(&mtx_a);
    MTX34_ctor(&mtx_b);

    _EFT_MODEL_WORK* work = self->work_0x38;
    s32 timer = --self->timer_0x0C;

    switch (self->state_0x06) {
    case 0:
        if (timer > 0) {
            return;
        }
        self->state_0x06++;
        self->timer_0x0C = 10;
        switch (self->type_0x02) {
        case 14:
            fn_800DB608(1, &self->pos_0x18, self->field_0x07);
            break;
        case 18:
            fn_800DB608(0, &self->pos_0x18, self->field_0x07);
            break;
        case 19:
            fn_800DB608(1, &self->pos_0x18, self->field_0x07);
            break;
        }
        return;
    case 1:
        if (timer < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        break;
    }

    change_paramscale_eff(work->effect, work->paramscale);
    if (effect_move(work->effect) != 0) {
        fn_800F93D8_view1(self, &work->effect, 1, work->count, 0);
    }
    self->frame_0x10++;

    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)work->model->field_0x118, 0);
        if (fn_800E2994(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            fn_8006F304(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx_b);
            mtx_b.m[0][3] = getKeyData(lbl_8059B760, (f32)self->frame_0x10);
            srt.SetEffectMtx(0, &mtx_b);
        }
    }
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)work->model->field_0x118, 1);
        if (fn_800E2994(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            fn_8006F304(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx_b);
            mtx_b.m[0][3] = getKeyData(lbl_8059B778, (f32)self->frame_0x10);
            srt.SetEffectMtx(0, &mtx_b);
        }
    }

    {
        u8* rgb_keys = 0;
        u8* alpha_keys = 0;

        switch (self->type_0x02) {
        case 14:
            rgb_keys = lbl_8059B6F8;
            alpha_keys = lbl_8059B708;
            break;
        case 18:
            rgb_keys = lbl_807916D0;
            alpha_keys = lbl_807916D8;
            break;
        case 19:
            rgb_keys = lbl_8059B71C;
            alpha_keys = lbl_8059B730;
            break;
        }
        if (rgb_keys != 0) {
            eftGetKeyRGB(rgb_keys, self->frame_0x10, &color.r, &color.g, &color.b);
            color.a = eftGetKeyAlpha(lbl_8059B744, self->frame_0x10);
            work->model->setMatColor(0, GX_COLOR0A0, color, false);
        }
        if (alpha_keys != 0) {
            eftGetKeyRGB(alpha_keys, self->frame_0x10, &color.r, &color.g, &color.b);
            color.a = eftGetKeyAlpha(lbl_8059B750, self->frame_0x10);
            work->model->setMatColor(1, GX_COLOR0A0, color, false);
        }
    }

    fn_800532DC(&mtx_a, get_current_view_mtx());
    mtx34_inverse(&mtx_a, &mtx_a);
    mtx34_identity(&mtx_b);
    fn_800FBB90(&mtx_b, &self->pos_0x18);
    mtx34_concat_assign(&mtx_b, &mtx_a);
    work->model->move2(&mtx_b, 0);
    fn_800F93D8_view1(self, &work->model, 2, 1, 0);
}

/* Spawns the eft002 effect for a player: builds the object, seeds its pool block with the parameter id
 * and the scale, and installs the two handlers. */
void eft002_set(_PLW* self, u8 type, u8 param, f32 scale)
{
    if (self->area_0x16 != (u8)get_now_areano()) {
        return;
    }
    _EFT* effect = fn_800F8788_view1(16);
    if (effect == 0) {
        return;
    }
    _EFT_WORK* work = effect->work_0x38;
    work->count = 1;
    work->param_id = param;
    work->scale = scale;
    effect->source_0x30 = self;
    effect->type_0x02 = type;
    effect->field_0x03 = 2;
    effect->timer_0x0C = 0;
    effect->flag_0x01 = 1;
    effect->area_0x44 = self->area_0x16;
    fn_800F9DF4_view1(effect, 0, 0);
    effect->release_0x40 = fn_800FD4A8;
    effect->dispatch_0x34 = fn_800FD4E4;
}

/* Spawns the eft002 effect for a shell: the same object, with the shell's scale and long parameter. */
void eft002_set_shell(_SHELL_W* self, u8 type, f32 scale, long id)
{
    if (self->area_0x08 != (u8)get_now_areano()) {
        return;
    }
    _EFT* effect = fn_800F8788_view1(16);
    if (effect == 0) {
        return;
    }
    _EFT_WORK* work = effect->work_0x38;
    work->count = 1;
    work->scale = scale;
    effect->source_0x30 = self;
    effect->type_0x02 = type;
    effect->timer_0x0C = id;
    effect->field_0x03 = 2;
    effect->area_0x44 = self->area_0x08;
    effect->flag_0x01 = 1;
    fn_800F9DF4_view1(effect, 0, 0);
    effect->release_0x40 = fn_800FD4A8;
    effect->dispatch_0x34 = fn_800FD4E4;
}

/* Releases the effect's pool block: hands every pooled effect back and clears the count. */
extern "C" void fn_800FD4A8(_EFT* self)
{
    _EFT_WORK* work = self->work_0x38;
    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->effect, work->count);
    work->count = 0;
}

/* Runs the effect's `byte_5` state handler. */
extern "C" void fn_800FD4E4(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_800FD520(self);
        break;
    case 1:
        fn_800FD718(self);
        break;
    case 2:
        fn_800FD850(self);
        break;
    case 3:
        fn_800FD860(self);
        break;
    }
}
