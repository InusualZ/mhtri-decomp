/* ef/eft002.cpp - the tail of the eft001 effect family (`eft001_set_pos` and its pooled-effect builders) and the
 *   eft002 family (`eft002_set`, `eft002_set_shell`, the model effect's per-frame body and its player gate).
 * RANGE. .text 0x800FBE64-0x800FD520 (24 functions); extab 0x8000BD8C-0x8000BDFC, extabindex 0x80025B3C-0x80025BE4,
 *   .data 0x8059B638-0x8059B9A8, .sdata 0x807916C8-0x807916E0, .sdata2 0x80796640-0x80796658.
 * FLAGS. `cflags_main`; file-scope `#pragma peephole off`, `optimization_level 4` (retail's switch lowering: the
 *   `addi`/`cmplwi` range tests of `fn_800FBE64`/`fn_800FCA54`; no other row moves), `pool_data off`, `fp_contract off`.
 * NAMES. `eft001_set_pos`, `eft002_set` and `eft002_set_shell` are the runtime dump's own names; `eft_rot_vec_copy`
 *   is a GUESS from its body; the map has only `fn_` stems for the rest, so plain definitions are `extern "C"`.
 *   GUESS (from the body): `eft_effect_foreach_pm_scale` (0x800FCEB0) walks the particle managers with the scale callback.
 *   GUESS (from what they hold and who reads them): the `eft001_res_*` and `eft002_*` tables, `pl_frame_eft_color_get`
 *   (STOPGAP ef-game3#19), the file-local `eft002_create` and `eft002_pm_release` (their own map rows, split off
 *   `fn_800FC428` and `fn_800FCEC8`).
 * RESIDUALS. None: every row matches and flipcheck is READY.
 * SHAPES. The two returned values consumed by pointer (`res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false))`,
 *   `mtx34_copy(&mtx_a, get_current_view_mtx())`) go through a reference parameter, so MWCC places the temporary
 *   below the named locals; a named local swaps the slots or copies the 48-byte matrix.
 *   `fn_800FD2B0`'s switch covers case 0 (a 0-based table, `cmplwi r0,29`) and the `flag != 1` return follows it;
 *   `_PLW::flag_0x30` is `s8` (retail `cmpwi`).
 *   `fn_800FCED4`'s inner type switch uses `break` plus one trailing `return` (a `return` per case adds a dead `b`);
 *   `state_0x06++` reuses the switch operand; `frame_0x10` is `s32` (retail's `xoris` int-to-float form).
 *   `_EFT_WORK` (the setters' `f32` scale and `u32` id) and `_EFT_MODEL_WORK` (`fn_800FCED4`'s `MHchar*` and
 *   `f32`) are two views of the 72-byte effect object's +0x38 block.
 *   The `<name>_viewN`/`<name>_cN` cast macros keep each folded source's view of a callee; a cast call is the same
 *   direct call.
 *   `fn_800FC428`/`fn_800FCEC8` tail-call a file-local function defined right after them (retail's case-0 `b`).
 *   `eft_water_state_ck` is read as `bool` and stored into a `bool` field (raw `stb` and `cmplwi`); a `u8` index into
 *   a flat `u8[20]` colour table gives retail's `add rI,rIdx,rBase`; `(s32)u8_field == 14` gives `cmpwi` without
 *   `extsb`; `u8` parameters stored raw and compared through `(u8)` keep retail's one `clrlwi`.
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
#include "ef/eft001.h" /* eft_rot_vec_copy, fn_800FC0F0 */
#undef fn_800FD2AC
#undef fn_800FD29C
#undef fn_800FCED4
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/fn_800FD520.h"
#include "ef/fn_800FD718.h"
#include "g3d/g3d_scnroot.h" /* VEC2_ctor (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* res_tex_srt_copy_ctor (rule 2) */
#include "g3d/g3d_state.h"
#include "unsplit/sound.h"
#include "menu/get_pop_dat_ptr.h" /* get_option_21 (rule 2) */
#include "fn_8004CAD8.h" /* cpSetRotMatrix (rule 2) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(void*, u8, u8))eft_state_flags_set)
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800F8914_c1 ((void* (*)())eft_res_model_get)
/* fn_800FD2AC_view1: the folded sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800FD2AC_view1 ((void (*)(struct _EFT*))fn_800FD2AC)
/* fn_800FD29C_view1: the folded sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800FD29C_view1 ((void (*)(struct _EFT*))fn_800FD29C)
/* fn_800FCED4_view1: the folded sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800FCED4_view1 ((void (*)(struct _EFT*))fn_800FCED4)
/* fn_800F9DF4_view1: the folded sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800F9DF4_view1 ((void (*)(_EFT*, u8, u8))eft_state_flags_set)
/* fn_800F93D8_view1: the folded sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800F93D8_view1 ((void (*)(void*, void*, u32, s32, u32))eft_res_models_spawn)
/* fn_800F8788_view1: the folded sources disagreed on this callee's parameters; calls keep their own view through a cast (same direct call). */
#define fn_800F8788_view1 ((_EFT* (*)(u32))eft_res_slot_get)

#pragma peephole off
#pragma optimization_level 4
#pragma pool_data off
#pragma fp_contract off

/* The `fn_800FBE64` / `fn_800FC484` family block: two pooled effects and the model they share. */
struct _EFT001_EFFECT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effect_0x04;  /* the primary pooled effect */
    /* +0x08 */ union {
        nw4r::ef::Effect* effect_0x08;  /* the secondary effect (types 20/21) */
        MHchar* model_0x08;             /* the joint model (the shell types 14/18/19) */
    };
    /* +0x0C */ f32 scale_0x0C;
    /* +0x10 */ u8 unused_0x10[4];
    /* +0x14 */ _GXColor color_0x14; /* the colour the frame hands change_color_eff */
    /* +0x18 */ union { f32 value_0x18; s8 timer_0x18; };
    /* +0x1C */ u8 unused_0x1C[0x0C];
    /* +0x28 */ bool water_0x28;     /* the water state the effect was created in */
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
    /* +0x00 */ u8 unused_0x00[0x06];
    /* +0x06 */ u8 field_0x06;     /* with kind 3, zero selects the small scale */
    /* +0x07 */ u8 unused_0x07[0x08];
    /* +0x0F */ u8 area_0x0F;      /* the area the builder is gated on */
    /* +0x10 */ struct _EFT001_SRC_OWNER* owner_0x10; /* the actor the effect takes its rotation from */
    /* +0x14 */ u8 unused_0x14[0x18];
    /* +0x2C */ struct _EFT001_SRC_SHELL* shell_0x2C; /* the equipped shell, read when kind_0x30 is 1 */
    /* +0x30 */ u8 kind_0x30;      /* 1 = a shell is equipped, 3 = the bare kind */
    /* +0x31 */ u8 unused_0x31[0x05];
    /* +0x36 */ u16 param_0x36;     /* the joint/parameter passed to the model builder */
    /* +0x38 */ u8 unused_0x38[0x04];
};

/* The actor `_EFT001_SRC::owner_0x10` points at: a word the effect keeps at +0x0C and the rotation triple
 * at +0x54.  size: 0x60 - lower bound, an approximation. */
struct _EFT001_SRC_OWNER {
    /* +0x00 */ u8 unused_0x00[0x0C];
    /* +0x0C */ void* handle_0x0C;  /* stored as the effect's source for type 12 */
    /* +0x10 */ u8 unused_0x10[0x44];
    /* +0x54 */ _CP_VECTOR rot_0x54;
};

/* The shell `_EFT001_SRC::shell_0x2C` points at: its type picks the scale and the colour set.
 * size: 0x0B - lower bound, an approximation. */
struct _EFT001_SRC_SHELL {
    /* +0x00 */ u8 unused_0x00[0x03];
    /* +0x03 */ u8 type_0x03;       /* 26..39: the shell types with their own colour set */
    /* +0x04 */ u8 unused_0x04[0x06];
    /* +0x0A */ u8 level_0x0A;      /* 3..4 select colour set 2 for type 32 */
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
void VEC2_ctor(void* p);

void fn_800DB6CC(nw4r::math::VEC3* pos);
void fn_800DB714(nw4r::math::VEC3* pos);
void fn_800DB75C(void* enemy, nw4r::math::VEC3* pos);

u32 fn_8029F564(void* self, u32 flag);

/* untyped: an opaque handle passed through (the pooled slot is only stored) */
void* eft_res_slot_get(u32 pool);
void eft_res_slot_release(void* self);
void eft_res_models_release(void* p, s32 mode);

void eft_res_models_spawn(void* self, nw4r::ef::Effect** effects, s32 count, s32 mode, void* arg);
bool eft_water_state_ck(void* self);
/* untyped: an opaque handle passed through (the effect record is only handed on) */

void fn_800AA75C(void* dst);
}

/* STOPGAP-BEGIN(ef-game3#19) */
extern "C" u8 pl_frame_eft_color_get(struct _EFT001_SRC_OWNER* owner);
/* STOPGAP-END(ef-game3#19) */

extern "C" _EFT* fn_800FC27C(nw4r::math::VEC3* pos, u32 type, u32 param, u8 area, f32 scale);
extern "C" void fn_800FCEC8(void* entry, u32 index);
extern "C" void fn_800FC384(_EFT* self);
extern "C" void fn_800FC428(_EFT* self);
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
extern "C" void mtx34_copy(void* dst, const nw4r::math::MTX34& src);
extern "C" void eft_res_slot_release(void* self);
extern "C" void fn_800FD4A8(_EFT* self);
extern "C" void fn_800FD4E4(_EFT* self);

nw4r::math::MTX34 get_current_view_mtx();

u8 eftGetKeyAlpha(u8* keys, long frame);

/* ---------------------------------------------------------------------------------------------------
 * the unit's tables (.data, .sdata), in address order
 * ------------------------------------------------------------------------------------------------- */

/* The res_eft_create effect id and parameter id of each effect type. */
u16 eft001_res_effect_ids[28] = {
    0x0000, 0x0001, 0x0002, 0x00DB, 0x00DC, 0x00DD, 0x00F0, 0x0103, 0x0103, 0x0127, 0x0128, 0x0184, 0x0287, 0x02C0,
    0x03B7, 0x03F5, 0x0410, 0x04BB, 0x0505, 0x0506, 0x0507, 0x0508, 0x0562, 0x0562, 0x0698, 0x0029, 0x0A43,
};
u16 eft001_res_param_ids[28] = {
    0x0002, 0x0002, 0x0002, 0x0018, 0x0018, 0x0018, 0x001C, 0x0021, 0x0021, 0x0027, 0x0027, 0x0015, 0x0015, 0x0025,
    0x0021, 0x0021, 0x001C, 0x0021, 0x0021, 0x0021, 0x0021, 0x0021, 0x0018, 0x0018, 0x0018, 0x0006, 0x0006,
};

/* The (frame, r, g, b) colour keys of the type-11 effect per colour set, ending at frame 0xFF; set 0 is also
 * type 20's dry key set, set 2 type 21's. */
u8 eft002_rgb_keys_0[12] = {0x00, 0xC4, 0x3F, 0xBF, 0x0F, 0x5F, 0x1F, 0xBF, 0xFF, 0x5F, 0x1F, 0xBF};
u8 eft002_rgb_keys_water[12] = {0x00, 0xC4, 0x3F, 0xCF, 0x0F, 0x5F, 0x1F, 0x7F, 0xFF, 0x5F, 0x1F, 0x7F};
u8 eft002_rgb_keys_1[12] = {0x00, 0xFF, 0xFF, 0xC3, 0x0F, 0xFF, 0xFF, 0x64, 0xFF, 0xFF, 0xFF, 0x64};
u8 eft002_rgb_keys_2[12] = {0x00, 0xFF, 0xFF, 0xFF, 0x0F, 0xBF, 0xBF, 0xFF, 0xFF, 0xBF, 0xBF, 0xFF};
u8 eft002_rgb_keys_3[12] = {0x00, 0xFF, 0x9F, 0x9F, 0x0F, 0xFF, 0x5F, 0x5F, 0xFF, 0xFF, 0x5F, 0x5F};
u8 eft002_rgb_keys_4[8] = {0x00, 0xA0, 0x80, 0x10, 0xFF, 0xA0, 0x80, 0x10};
u8* eft002_rgb_key_sets[5] = {
    eft002_rgb_keys_0, eft002_rgb_keys_1, eft002_rgb_keys_2, eft002_rgb_keys_3, eft002_rgb_keys_4,
};

/* The model's two material colours (`fn_800FCED4`): (frame, r, g, b) keys per material for the shell types 14,
 * 18 and 19, the (frame, alpha) keys and the texture-scroll (frame, value) keys, -1 ending. */
u8 eft002_mat0_rgb_keys_14[16] = {0x00, 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0x04, 0x50, 0x50, 0xFF,
                                  0xFF, 0x50, 0x50, 0xFF};
u8 eft002_mat1_rgb_keys_14[20] = {0x00, 0xFF, 0xFF, 0xFF, 0x06, 0xFF, 0xFF, 0xFF, 0x08, 0x50, 0x50, 0xFF,
                                  0x0A, 0x50, 0x50, 0xFF, 0xFF, 0x50, 0x50, 0xFF};
u8 eft002_mat0_rgb_keys_19[20] = {0x00, 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF, 0x28,
                                  0x06, 0xFF, 0x10, 0x00, 0xFF, 0xFF, 0x10, 0x00};
u8 eft002_mat1_rgb_keys_19[20] = {0x00, 0xFF, 0xFF, 0xFF, 0x06, 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0xFF, 0x28,
                                  0x0A, 0xFF, 0x10, 0x00, 0xFF, 0xFF, 0x10, 0x00};
u8 eft002_mat0_alpha_keys[12] = {0x00, 0xFF, 0x03, 0xFF, 0x04, 0x61, 0x06, 0x10, 0xFF, 0x00};
u8 eft002_mat1_alpha_keys[16] = {0x00, 0xFF, 0x06, 0xFF, 0x08, 0x4C, 0x0A, 0x10, 0xFF, 0x10};
f32 eft002_mat0_scroll_keys[6] = {0.0f, 0.6f, 6.0f, 0.0f, -1.0f, 0.0f};
f32 eft002_mat1_scroll_keys[8] = {0.0f, 0.5f, 2.0f, 0.5f, 9.0f, 0.0f, -1.0f, 0.0f};
u8 eft002_mat0_rgb_keys_18[8] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};
u8 eft002_mat1_rgb_keys_18[8] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00};

/* The (frame, r, g, b) colour keys of the types 25/26 per colour set. */
u8 eft002_rgb_keys_25_0[12] = {0x00, 0xFF, 0xFF, 0xFF, 0x06, 0xFF, 0x4F, 0x4F, 0xFF, 0xFF, 0x4F, 0x4F};
u8 eft002_rgb_keys_25_1[12] = {0x00, 0xFF, 0xFF, 0xFF, 0x06, 0x3C, 0xBE, 0xFF, 0xFF, 0x3C, 0xBE, 0xFF};
u8 eft002_rgb_keys_25_2[12] = {0x00, 0xFF, 0xFF, 0xFA, 0x06, 0x5A, 0xBE, 0xFF, 0xFF, 0x5A, 0xBE, 0xFF};
u8 eft002_rgb_keys_25_3[12] = {0x00, 0xFF, 0xFF, 0xFF, 0x06, 0xFF, 0x00, 0xA0, 0xFF, 0xFF, 0x00, 0xA0};
u8* eft002_rgb_key_sets_25[4] = {
    eft002_rgb_keys_25_0, eft002_rgb_keys_25_1, eft002_rgb_keys_25_2, eft002_rgb_keys_25_3,
};

/* The RGBA of the types 0..2 per colour set, out of and in the water. */
u8 eft002_colors[20] = {0x08, 0x28, 0x08, 0xFF, 0x08, 0x2D, 0x0A, 0xFF, 0x0C, 0x23, 0x46, 0xFF,
                        0xE6, 0xD8, 0xE2, 0xFF, 0x28, 0x0A, 0x00, 0xFF};
u8 eft002_water_colors[20] = {0x08, 0x28, 0x08, 0xFF, 0x08, 0x2D, 0x0A, 0xFF, 0x0C, 0x23, 0x46, 0xFF,
                              0xE6, 0xD8, 0xE2, 0xFF, 0x40, 0x0A, 0x00, 0xFF};

extern "C" void fn_800FD2AC(void* self);
extern "C" void fn_800FD29C(_EFT_MODEL* self);
extern "C" void fn_800FCED4(_EFT_MODEL* self);

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Builds a pooled two-effect record at `pos` for an actor in the current area: the scale and colour set come from
 * the actor's equipped shell, the rotation (or source handle) from its owner for the types that follow it. */
extern "C" void fn_800FBE64(nw4r::math::VEC3* pos, u32 type, _EFT001_SRC* src, u8 color_set)
{
    _EFT001_EFFECT_WORK* work;
    _EFT* effect;

    if (src->area_0x0F != (u8)get_now_areano()) {
        return;
    }
    if (type == 16 && fn_800FD2B0((_PLW*)src) == 0) {
        return;
    }
    effect = (_EFT*)eft_res_slot_get(44);
    if (effect == NULL) {
        return;
    }
    work = (_EFT001_EFFECT_WORK*)effect->work_0x38;
    work->count = 1;
    if (type - 25 <= 1) {
        color_set = pl_frame_eft_color_get(src->owner_0x10);
    }
    effect->type_0x02 = type;
    effect->demo_flag_0x08 = color_set;
    effect->field_0x03 = 1;
    effect->timer_0x0C = 0;
    copyVec3(&effect->pos_0x18, pos);
    effect->area_0x44 = src->area_0x0F;
    if (src->kind_0x30 == 1) {
        _EFT001_SRC_SHELL* shell = src->shell_0x2C;
        switch (shell->type_0x03) {
        case 26:
        case 30:
        case 31:
            work->value_0x18 = 0.5f;
            effect->field_0x07 = 4;
            break;
        case 36:
        case 37:
        case 38:
        case 39:
            work->value_0x18 = 0.5f;
            effect->field_0x07 = 0;
            break;
        case 33:
            work->value_0x18 = 0.5f;
            effect->field_0x07 = 1;
            break;
        case 32:
            work->value_0x18 = 1.0f;
            if ((u32)(shell->level_0x0A - 3) <= 1) {
                effect->field_0x07 = 2;
            } else {
                effect->field_0x07 = 4;
            }
            break;
        case 35:
            work->value_0x18 = 1.0f;
            effect->field_0x07 = 3;
            break;
        default:
            work->value_0x18 = 1.0f;
            effect->field_0x07 = 4;
            break;
        }
    } else {
        if (src->kind_0x30 == 3 && src->field_0x06 == 0) {
            work->value_0x18 = 0.5f;
        } else {
            work->value_0x18 = 1.0f;
        }
        effect->field_0x07 = 4;
    }
    if (fn_8029F564(src, 0x800) == 1) {
        work->flag_0x29 = 1;
    } else {
        work->flag_0x29 = 0;
    }
    switch (effect->type_0x02) {
    case 22:
    case 23:
        eft_rot_vec_copy(&effect->rot_0x24, &src->owner_0x10->rot_0x54);
        break;
    case 25:
    case 26:
        eft_rot_vec_copy(&effect->rot_0x24, &src->owner_0x10->rot_0x54);
        break;
    case 12:
        effect->source_0x30 = src->owner_0x10->handle_0x0C;
        break;
    }
    eft_state_flags_set_c1(effect, 0, 4);
    effect->release_0x40 = fn_800FC384;
    effect->dispatch_0x34 = fn_800FC428;
}

/* Copies a 12-byte rotation triple. */
extern "C" void eft_rot_vec_copy(_CP_VECTOR* dst, _CP_VECTOR* src)
{
    *dst = *src;
}

/* Builds a model effect with a full parameter set: the area gate, the model builder, the effect's
 * `field_0x08`/`field_0x06` seed bytes, the rotation triple and the parameter scale. */
extern "C" void fn_800FC0F0(nw4r::math::VEC3* pos, u32 type, u8 field_08, u8 area, _CP_VECTOR* rot, f32 scale)
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
    effect->demo_flag_0x08 = field_08;
    effect->field_0x03 = 1;
    effect->timer_0x0C = 0;
    copyVec3(&effect->pos_0x18, pos);
    eft_rot_vec_copy(&effect->rot_0x24, rot);
    effect->area_0x44 = area;
    work->value_0x18 = scale;
    effect->field_0x07 = 4;
    effect->field_0x06 = 0;
    eft_state_flags_set_c1(effect, 0, 4);
    effect->release_0x40 = fn_800FC384;
    effect->dispatch_0x34 = fn_800FC428;
}

/* Builds a model effect from a joint source: its +0x36 word and +0x0F area seed `fn_800FC27C`, and the
 * effect's `field_0x07` records whether the source owns the 0x800 flag. */
extern "C" void fn_800FC1DC(nw4r::math::VEC3* pos, u32 type, _EFT001_SRC* src)
{
    _EFT* effect = fn_800FC27C(pos, type, src->param_0x36, src->area_0x0F, 1.0f);

    if (effect != NULL) {
        if (fn_8029F564(src, 2048) == 1) {
            effect->field_0x07 = 1;
        } else {
            effect->field_0x07 = 0;
        }
    }
}

/* Wrapper around `fn_800FC27C` for a `_CP_VECTOR` rotation triple: its second word is the parameter
 * stored on the effect. */
void eft001_set_pos(nw4r::math::VEC3* pos, u32 a, _CP_VECTOR* v, u8 b, f32 c)
{
    if (fn_800FC27C(pos, a, v->y, b, c) == NULL) {
        return;
    }
}

/* Builds a model effect at `pos`: gated on the current area, allocates a 16-entry pool effect,
 * installs the two per-frame handlers, seeds the model list and the parameter scale. */
extern "C" _EFT* fn_800FC27C(nw4r::math::VEC3* pos, u32 type, u32 param, u8 area, f32 scale)
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
    effect->area_0x44 = area;
    eft_state_flags_set_c1(effect, 0, 4);
    return effect;
}

/* Dispatches to the two per-frame model handlers by the effect type. */
extern "C" void fn_800FC384(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s32)self->type_0x02 == 14) {
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

    eft_res_models_release(&work->effect_0x08, 1);
    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->effect_0x04, work->count);
    work->count = 0;
}

/* The per-frame dispatcher: advance the state handler the effect's `byte_5` selects. */
static void eft002_create(_EFT* self);

extern "C" void fn_800FC428(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft002_create(self);
        break;
    case 1:
        fn_800FCA34(self);
        break;
    case 2:
        fn_800FD29C_view1(self);
        break;
    case 3:
        fn_800FD2AC_view1(self);
        break;
    }
}

/* State 0: runs the model builder of the shell types 14, 18 and 19, the effect builder of the others. */
static void eft002_create(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s32)self->type_0x02 == 14) {
        fn_800FC7EC(self);
    } else {
        fn_800FC484(self);
    }
}

/* Creates the type's pooled effect (and the second effect of types 20/21), places and orients it, and picks its
 * colour and scale from the water state and the colour set before the first frame runs. */
extern "C" void fn_800FC484(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    _EFT001_EFFECT_WORK* work;
    s32 i;

    MTX34_ctor(&mtx);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    work = (_EFT001_EFFECT_WORK*)self->work_0x38;
    self->state_0x05++;

    work->effect_0x04 = res_eft_create(eft001_res_effect_ids[self->type_0x02], eft001_res_param_ids[self->type_0x02], 0);
    if (work->effect_0x04 == NULL) {
        fn_800FD2AC_view1(self);
        return;
    }

    switch (self->type_0x02) {
    case 7:
    case 8:
        self->timer_0x0C = 5;
        break;
    case 11:
        if (self->demo_flag_0x08 == 0) {
            work->water_0x28 = eft_water_state_ck(self);
        }
        break;
    case 15:
        self->timer_0x0C = 5;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 17:
        self->timer_0x0C = 5;
        break;
    case 20:
    case 21:
        self->timer_0x0C = 5;
        work->effect_0x08 = res_eft_create(388, 21, 0);
        if (work->effect_0x08 == NULL) {
            fn_800FD2AC_view1(self);
            return;
        }
        work->count++;
        if (self->type_0x02 == 20) {
            work->water_0x28 = eft_water_state_ck(self);
        }
        break;
    case 22:
    case 23:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    case 25:
    case 26:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        work->effect_0x04->SetRootMtx(mtx);
        break;
    }

    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans((&work->effect_0x04)[i], &self->pos_0x18);
    }
    self->flag_0x01 = 1;

    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2:
        if (eft_water_state_ck(self) == 1) {
            work->color_0x14.r = eft002_water_colors[self->field_0x07 * 4];
            work->color_0x14.g = eft002_water_colors[self->field_0x07 * 4 + 1];
            work->color_0x14.b = eft002_water_colors[self->field_0x07 * 4 + 2];
            work->color_0x14.a = eft002_water_colors[self->field_0x07 * 4 + 3];
        } else {
            work->color_0x14.r = eft002_colors[self->field_0x07 * 4];
            work->color_0x14.g = eft002_colors[self->field_0x07 * 4 + 1];
            work->color_0x14.b = eft002_colors[self->field_0x07 * 4 + 2];
            work->color_0x14.a = eft002_colors[self->field_0x07 * 4 + 3];
        }
        if (get_option_21() != 0) {
            work->value_0x18 *= 0.5f;
        }
        break;
    case 3:
    case 4:
    case 5:
        work->value_0x18 *= 0.5f;
        break;
    case 24:
        if (eft_water_state_ck(self) == 1) {
            work->color_0x14.r = 100;
            work->color_0x14.g = 191;
            work->color_0x14.b = 255;
            work->color_0x14.a = 255;
        } else {
            work->color_0x14.r = 255;
            work->color_0x14.g = 255;
            work->color_0x14.b = 255;
            work->color_0x14.a = 255;
        }
        break;
    default:
        work->color_0x14.a = 255;
        break;
    }
    self->field_0x06 = 0;
    fn_800FCA34(self);
}

/* Builds the pooled effect and model pair from the type's id tables, poses the model, offsets its animation clock at
 * random and seeds the two K-colours. */
extern "C" void fn_800FC7EC(_EFT* self)
{
    _EFT001_EFFECT_WORK* work = (_EFT001_EFFECT_WORK*)self->work_0x38;
    _GXColor color;

    self->state_0x05++;

    work->effect_0x04 = res_eft_create(eft001_res_effect_ids[self->type_0x02], eft001_res_param_ids[self->type_0x02], 0);
    if (work->effect_0x04 == NULL) {
        fn_800FD2AC_view1(self);
        return;
    }

    if (res_eft_model_create(work->model_0x08, 82, 340) == NULL) {
        fn_800FD2AC_view1(self);
        return;
    }

    SetRootMtxTrans(work->effect_0x04, &self->pos_0x18);
    copyVec3(&work->model_0x08->pos_0x04, &self->pos_0x18);
    eft_rot_vec_copy(&work->model_0x08->rot_0x28, &self->rot_0x24);

    work->model_0x08->field_0x2C -= 3641;
    work->model_0x08->field_0x2C += (u16)ran_suu(0) & 0x1FFF;
    work->model_0x08->field_0x30 = 0xF1C7;
    work->model_0x08->field_0x30 += (u16)ran_suu(0) & 0x1FFF;

    work->model_0x08->setVisibility(1, false);
    setVector3(&work->model_0x08->scale_0x1C, work->scale_0x0C, work->scale_0x0C, work->scale_0x0C);

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
                work->model_0x08->setMatAlphaBlendMode(i, GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
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
            work->model_0x08->setTevKColor(i, GX_KCOLOR3, &color);
        }
    }

    self->field_0x06 = 0;
    fn_800FCA34(self);
}

/* Dispatches to the two per-frame model handlers of the second half of the unit. */
extern "C" void fn_800FCA34(_EFT* self)
{
    if ((u32)(self->type_0x02 - 18) <= 1 || (s32)self->type_0x02 == 14) {
        fn_800FCED4_view1(self);
    } else {
        fn_800FCA54(self);
    }
}

/* The per-frame body of the non-model (type 11..26) effect: advances the two-stage machine, moves
 * the pooled effects, and recolours them per type from the key tables. */
extern "C" void fn_800FCA54(_EFT* self)
{
    nw4r::math::VEC2 scale;
    nw4r::math::VEC3 vec;
    _EFT001_EFFECT_WORK* work;
    s32 i;
    u8 live;

    VEC2_ctor(&scale);
    VEC3_ctor(&vec);
    live = 0;
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
        scale.x = 0.5f;
        scale.y = 0.5f;
        eft_effect_foreach_pm_scale(work->effect_0x04, (u32)&scale, true);
    }

    switch (self->type_0x02) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 9:
    case 10:
    case 22:
    case 26:
        change_paramscale_eff(work->effect_0x04, work->value_0x18);
        break;
    case 25:
        setVector3(&vec, work->value_0x18, work->value_0x18, 1.5f * work->value_0x18);
        change_paramscale_eff_vec3(work->effect_0x04, &vec);
        break;
    }

    for (i = 0; i < work->count; i++) {
        if (effect_move((&work->effect_0x04)[i]) == 0) {
            live++;
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
        change_color_eff(work->effect_0x04, &self->pos_0x18, work->color_0x14);
        break;
    case 7:
    case 8:
        if (eft_water_state_ck(self) == 1) {
            work->color_0x14.r = 100;
            work->color_0x14.g = 191;
            work->color_0x14.b = 255;
            work->color_0x14.a = 255;
            change_color_eff(work->effect_0x04, &self->pos_0x18, work->color_0x14);
        }
        break;
    case 11:
        eftGetKeyRGB(work->water_0x28 == 0 ? eft002_rgb_key_sets[self->demo_flag_0x08] : eft002_rgb_keys_water,
                     self->field_0x10, &work->color_0x14.r, &work->color_0x14.g, &work->color_0x14.b);
        change_color_eff(work->effect_0x04, &self->pos_0x18, work->color_0x14);
        break;
    case 20:
        if (work->water_0x28 == 0) {
            eftGetKeyRGB(eft002_rgb_keys_0, self->field_0x10, &work->color_0x14.r, &work->color_0x14.g,
                         &work->color_0x14.b);
        } else {
            _GXColor color;
            color.r = 255;
            color.g = 175;
            color.b = 175;
            color.a = 255;
            change_color_eff(work->effect_0x04, &self->pos_0x18, color);
            eftGetKeyRGB(eft002_rgb_keys_water, self->field_0x10, &work->color_0x14.r, &work->color_0x14.g,
                         &work->color_0x14.b);
        }
        change_color_eff(work->effect_0x08, &self->pos_0x18, work->color_0x14);
        break;
    case 21:
        eftGetKeyRGB(eft002_rgb_keys_2, self->field_0x10, &work->color_0x14.r, &work->color_0x14.g,
                     &work->color_0x14.b);
        change_color_eff(work->effect_0x08, &self->pos_0x18, work->color_0x14);
        break;
    case 25:
    case 26:
        eftGetKeyRGB(eft002_rgb_key_sets_25[self->demo_flag_0x08], self->field_0x10, &work->color_0x14.r,
                     &work->color_0x14.g, &work->color_0x14.b);
        change_color_eff(work->effect_0x04, &self->pos_0x18, work->color_0x14);
        break;
    }
    eft_res_models_spawn(self, (nw4r::ef::Effect**)&work->effect_0x04, 1, work->count, NULL);
}

/* Walks the effect's particle-manager pool through `ForeachParticleManager`. */
extern "C" void eft_effect_foreach_pm_scale(nw4r::ef::Effect* self, u32 arg, bool flag)
{
    self->ForeachParticleManager(fn_800FCEC8, arg, flag);
}

/* The particle-manager walk callback: forwards the entry (offset by its 88-byte header) to the shared
 * particle release. */
static void eft002_pm_release(_EFT001_PM_ENTRY* entry);

extern "C" void fn_800FCEC8(void* entry, u32 index)
{
    eft002_pm_release((_EFT001_PM_ENTRY*)entry);
}

/* Hands the particle manager behind the entry's 88-byte header to the shared particle release. */
static void eft002_pm_release(_EFT001_PM_ENTRY* entry)
{
    fn_800AA75C(&entry->field_0x58);
}

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
            res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx_b);
            mtx_b.m[0][3] = getKeyData(eft002_mat0_scroll_keys, (f32)self->frame_0x10);
            srt.SetEffectMtx(0, &mtx_b);
        }
    }
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)work->model->field_0x118, 1);
        if (fn_800E2994(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx_b);
            mtx_b.m[0][3] = getKeyData(eft002_mat1_scroll_keys, (f32)self->frame_0x10);
            srt.SetEffectMtx(0, &mtx_b);
        }
    }

    {
        u8* rgb_keys = 0;
        u8* alpha_keys = 0;

        switch (self->type_0x02) {
        case 14:
            rgb_keys = eft002_mat0_rgb_keys_14;
            alpha_keys = eft002_mat1_rgb_keys_14;
            break;
        case 18:
            rgb_keys = eft002_mat0_rgb_keys_18;
            alpha_keys = eft002_mat1_rgb_keys_18;
            break;
        case 19:
            rgb_keys = eft002_mat0_rgb_keys_19;
            alpha_keys = eft002_mat1_rgb_keys_19;
            break;
        }
        if (rgb_keys != 0) {
            eftGetKeyRGB(rgb_keys, self->frame_0x10, &color.r, &color.g, &color.b);
            color.a = eftGetKeyAlpha(eft002_mat0_alpha_keys, self->frame_0x10);
            work->model->setMatColor(0, GX_COLOR0A0, color, false);
        }
        if (alpha_keys != 0) {
            eftGetKeyRGB(alpha_keys, self->frame_0x10, &color.r, &color.g, &color.b);
            color.a = eftGetKeyAlpha(eft002_mat1_alpha_keys, self->frame_0x10);
            work->model->setMatColor(1, GX_COLOR0A0, color, false);
        }
    }

    mtx34_copy(&mtx_a, get_current_view_mtx());
    mtx34_inverse(&mtx_a, &mtx_a);
    mtx34_identity(&mtx_b);
    mtx34_set_trans(&mtx_b, &self->pos_0x18);
    mtx34_concat_assign(&mtx_b, &mtx_a);
    work->model->move2(&mtx_b, 0);
    fn_800F93D8_view1(self, &work->model, 2, 1, 0);
}

/* Bumps the effect's `byte_5` state index. */
extern "C" void fn_800FD29C(_EFT_MODEL* self)
{
    self->state_0x05++;
}

/* Destroys an effect object: runs the pool release handler, hands the pool block back, then clears the
 * object. */
extern "C" void fn_800FD2AC(void* self)
{
    eft_res_slot_release(self);
}

/* Answers whether the player's equipped shell type is one of the 22 the effect family supports: 1 for
 * those, 0 for everything else (and for a player without a shell loaded). */
extern "C" u32 fn_800FD2B0(_PLW* self)
{
    if ((s32)(u8)self->flag_0x30 == 1) {
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
