/* auto/800FCED4_fn_800FCED4.cpp - the `eft002` effect cluster, 0x800FCED4..0x800FD520 (8 functions).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * Two effect families share the range. `eft002_set` / `eft002_set_shell` (0x800FD2FC, 0x800FD3D8) build
 * the 72-byte effect object `fn_800F886C` destroys and install the two handlers that travel with it
 * (`fn_800FD4E4` = the `byte_5` dispatcher, `fn_800FD4A8` = the pool release). `fn_800FCED4` /
 * `fn_800FD29C` / `fn_800FD2AC` / `fn_800FD2B0` are the other family: the per-frame body, its `byte_5`
 * bump, the destructor thunk (`fn_800FD2AC` -> `fn_800F886C`, called from four sites in the unit before
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

#include "types.h"

/* ---------------------------------------------------------------------------------------------------
 * the Dolphin types the material calls take
 * ------------------------------------------------------------------------------------------------- */

/* The channel selector `setMatColor` takes; the call sites pass `GX_COLOR0A0` (4). */
enum _GXChannelID {
    GX_COLOR0,
    GX_COLOR1,
    GX_ALPHA0,
    GX_ALPHA1,
    GX_COLOR0A0,
    GX_COLOR1A1,
    GX_COLORZERO,
    GX_ALPHA0A0,
    GX_ALPHA1A1,
    GX_ALPHAZERO
};

/* The 4-byte colour `setMatColor` takes by value. size: 0x04 */
struct _GXColor {
    /* +0x00 */ u8 r;
    /* +0x01 */ u8 g;
    /* +0x02 */ u8 b;
    /* +0x03 */ u8 a;
};

/* ---------------------------------------------------------------------------------------------------
 * the nw4r engine types this unit calls through
 * ------------------------------------------------------------------------------------------------- */

namespace nw4r {
namespace math {
struct VEC3 { /* size: 0x0C */
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
};
struct MTX34 { /* size: 0x30 */
    /* +0x00 */ f32 m[3][4];
};
}  // namespace math

namespace ef {
struct Effect;
}  // namespace ef

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
}  // namespace g3d
}  // namespace nw4r

/* The game's character object: only the `ScnMdl` pointer at +0x118 is read here (to build the material
 * access). size: 0x11C - lower bound, an approximation. */
struct MHchar {
    /* +0x000 */ u8 unused_0x000[0x118];
    /* +0x118 */ nw4r::g3d::ScnMdl* scnmdl_0x118;

    void setMatColor(u32 idx, _GXChannelID channel, _GXColor color, bool unk);
    void move2(nw4r::math::MTX34* mtx, u16 unk);
};

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

/* The player work `eft002_set` takes. Only the offsets this unit reads are named; the full type is
 * `Pl/pl_act.cpp`'s `_PLW` and the two belong in one header. size: 0x668 */
struct _PLW {
    /* +0x000 */ u8 unused_0x000[0x16];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 unused_0x017[0x2C - 0x17];
    /* +0x02C */ _SHELL_W* equip_0x2C;
    /* +0x030 */ s8 flag_0x30;
    /* +0x031 */ u8 unused_0x031[0x668 - 0x31];
};

/* ---------------------------------------------------------------------------------------------------
 * the 72-byte effect object and its two pool-block views
 * ------------------------------------------------------------------------------------------------- */

/* Pool block of the eft002 family: the setters seed the effect count, the parameter id and the scale. */
struct _EFT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effect;
    /* +0x08 */ f32 scale;
    /* +0x0C */ u32 param_id;
};
/* size: 0x10 - lower bound, an approximation (the pool block the eft002 handlers walk). */

/* The effect object `eft002_set` / `eft002_set_shell` allocate and `fn_800F886C` destroys. */
struct _EFT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 unused_0x06;
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ u8 unused_0x08[0x0C - 0x08];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ u8 unused_0x10[0x18 - 0x10];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ void* source_0x30;
    /* +0x34 */ void (*dispatch_0x34)(_EFT*);
    /* +0x38 */ _EFT_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(_EFT*);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
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

extern "C" void fn_8005050C(void* mtx);
extern "C" void fn_800504D4(void* mtx);
extern "C" void fn_800532DC(void* dst, const nw4r::math::MTX34& src);
extern "C" void fn_800883C4(void* dst, void* src);
extern "C" void fn_80051574(void* dst, void* src);
extern "C" void fn_800DB608(u8 flag, nw4r::math::VEC3* pos, u8 arg);
extern "C" void fn_800FBB90(void* mtx, nw4r::math::VEC3* pos);
extern "C" void fn_800F93D8(void* self, void* list, u32 mode, s32 count, u32 arg);
extern "C" int fn_800E2994(void* handle);
extern "C" void fn_8006F304(void* dst, const u32& src);
extern "C" _EFT* fn_800F8788(u32 pool_id);
extern "C" void fn_800F9DF4(_EFT* self, u8 a, u8 b);
extern "C" void fn_800F886C(void* self);
extern "C" void fn_800FD4A8(_EFT* self);
extern "C" void fn_800FD4E4(_EFT* self);
extern "C" void fn_800FD520(_EFT* self);
extern "C" void fn_800FD718(_EFT* self);
extern "C" void fn_800FD850(_EFT* self);
extern "C" void fn_800FD860(_EFT* self);

u32 get_now_areano();
nw4r::math::MTX34 get_current_view_mtx();
f32 getKeyData(f32* keys, f32 frame);
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b);
u8 eftGetKeyAlpha(u8* keys, long frame);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
u32 effect_move(nw4r::ef::Effect* effect);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);

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
extern "C" f64 lbl_80796650[1];       /* the target's name for MWCC's int->float magic; our object pools its own copy */
extern "C" u32 jumptable_8059B930[30]; /* the target's name for fn_800FD2B0's switch table; our object emits its own */

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Destroys an effect object: runs the pool release handler, hands the pool block back, then clears the
 * object. */
extern "C" void fn_800FD2AC(void* self)
{
    fn_800F886C(self);
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

/* Retail was built with the peephole pass off for this half of the unit: with it on MWCC fuses the
 * u8->u32 zero-extend into the store (`clrlwi` dropped in eft002_set), folds the `lis`+`addi` address
 * materialisation into one register (r3 where retail has r0) and re-colours the key-table `lis` scratch.
 * fn_800FCED4 99.92 -> 100, eft002_set 97.73 -> 100, eft002_set_shell 99.62 -> 100. The pragma starts
 * after `fn_800FD2B0` on purpose: with the peephole off its 30-entry switch grows 4 bytes and drops to
 * 94.74 %, so that one function keeps the pass on. */
#pragma peephole off

/* Per-frame body of the effect: advances the two-step state machine, drives the texture-SRT animation on
 * both materials, recolours them from the key tables, and re-places the model. */
extern "C" void fn_800FCED4(_EFT_MODEL* self)
{
    nw4r::math::MTX34 mtx_a;
    nw4r::math::MTX34 mtx_b;
    _GXColor color;

    fn_8005050C(&mtx_a);
    fn_8005050C(&mtx_b);

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
        fn_800F93D8(self, &work->effect, 1, work->count, 0);
    }
    self->frame_0x10++;

    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(work->model->scnmdl_0x118, 0);
        if (fn_800E2994(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            fn_8006F304(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx_b);
            mtx_b.m[0][3] = getKeyData(lbl_8059B760, (f32)self->frame_0x10);
            srt.SetEffectMtx(0, &mtx_b);
        }
    }
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access(work->model->scnmdl_0x118, 1);
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
    fn_800883C4(&mtx_a, &mtx_a);
    fn_800504D4(&mtx_b);
    fn_800FBB90(&mtx_b, &self->pos_0x18);
    fn_80051574(&mtx_b, &mtx_a);
    work->model->move2(&mtx_b, 0);
    fn_800F93D8(self, &work->model, 2, 1, 0);
}

/* Spawns the eft002 effect for a player: builds the object, seeds its pool block with the parameter id
 * and the scale, and installs the two handlers. */
void eft002_set(_PLW* self, u8 type, u8 param, f32 scale)
{
    if (self->area_0x16 != (u8)get_now_areano()) {
        return;
    }
    _EFT* effect = fn_800F8788(16);
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
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_800FD4A8;
    effect->dispatch_0x34 = fn_800FD4E4;
}

/* Spawns the eft002 effect for a shell: the same object, with the shell's scale and long parameter. */
void eft002_set_shell(_SHELL_W* self, u8 type, f32 scale, long id)
{
    if (self->area_0x08 != (u8)get_now_areano()) {
        return;
    }
    _EFT* effect = fn_800F8788(16);
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
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_800FD4A8;
    effect->dispatch_0x34 = fn_800FD4E4;
}

/* Releases the effect's pool block: hands every pooled effect back and clears the count. */
extern "C" void fn_800FD4A8(_EFT* self)
{
    _EFT_WORK* work = self->work_0x38;
    push_eft_effect_heap_num(&work->effect, work->count);
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

#pragma peephole reset
