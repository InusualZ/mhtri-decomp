/* ef/eft026_fx.cpp - the eft026 enemy-fold family (`eft026_set` and two sibling setters, the allocator `fn_80117FF8`
 *   stamping tag 26, the release `fn_80118154` and its state machine) and the eft028 break/crumble family's setters
 *   (`eft028_set`, `eft028_set_koware`, `eft028_set_capsule`, `eft028_set_scaled`), which build their records through
 *   `ef/fn_80119C44.c`.
 * RANGE. .text 0x80117DA8-0x80119C44 (20 functions); extab 0x8000C4D4-0x8000C554, extabindex 0x80026628-0x800266E8,
 *   .data 0x805A0488-0x805A06F0, .sdata 0x80791940-0x80791970, .sdata2 0x80796AC0-0x80796B2C.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (with the pass on MWCC fuses the nested slot loop's
 *   `subi`/`cmpwi` into `subic.` and compresses the frame; playbook 39).
 * NAMES. `eft026_set` and `eft028_set_koware` are the runtime dump's own names; the map has only `fn_` stems for the
 *   rest, so plain definitions are `extern "C"`.
 *   GUESS (from its body and its caller): eft028_set_scaled; the data (map `lbl_805A0488`..`lbl_805A06B0`,
 *   `lbl_80791940`..`lbl_80791968`): `eft026_effect_ids`, `eft026_handler_kinds`, `eft026_durations`,
 *   `eft026_spin_rates`, `eft026_scale_keys_2`/`_9`/`_a`/`_b`, `eft026_alpha_keys_N`/`_kind2`,
 *   `eft026_alpha_key_tables`, `eft026_model_counts`; `EftUvModelHooks` (`update`, `set_scalar`).
 *   Also: `#pragma pool_data off` (each table its own `lis`/`addi`); `fn_80118B2C` and `fn_80118214` at
 *   `optimization_level 4` (their type switches' `subi`/`cmplwi` range test).
 * RESIDUALS. 2 partial rows:
 *  - `fn_8011870C`, `fn_80119450`: frames 0x10/0x20 larger (the stack slots of the material-access and colour
 *    temporaries are laid out differently), which also recolours `fn_8011870C`'s callee-saved registers.
 *   `.sdata2` is 0x68 of 0x6C: retail has a second, unreferenced 0.0f before 410.0f (0x80796B24) that one TU's
 *   pool cannot hold twice; `.data` 0x264 of 0x268 (identical prefix).
 *   Relocation names that differ from retail: pool constants (`@NNN`), the switch table (`jumptable_805A06BC`).
 * SHAPES. `_EFT26_PHASE`'s colour is a `_GXColor` member (byte-aligned, so `slots` stays at +0x7C); the phase loops
 *   index `work->phase[i]` (a `p` pointer adds an induction register) and count `done` in a `u8`.
 *   `eft026_set` is defined first (retail's `.text` and pool order).  The handlers load `work`/`source` after the
 *   constructors, extract colour bytes masked (`(col & 0xFF000000) >> 24`) and test loop floats against literals.
 *   `eft028_set_capsule` returns nothing; `fn_80117FF8`'s flag test re-reads the stored type; `_EFT26_WORK::slots` is
 *   `[2][2]`; `fn_80118FF0` runs `move2` before the scale copy.
 *   `fn_80118214`: the kind-1/2 vs 8/9 colours and the 1/8 vs 2/9 tail are `switch`es; the dead-enemy path stores
 *   state 3; the camera position is a `const VEC3&` bound after `fn_80119818`, with `cam` declared after it; the
 *   `== 8` scale test is `(s32)` (retail `cmpwi`).
 *   `eft026_durations` is `s32[]`: the eft026 timer tests compile to retail's signed `cmpw` only with the signed view.
 */

#include "ef/eft_res_spawn_gate_ck.h" /* eft_res_spawn_gate_ck (rule 2: the owner's header) */
#include "enemy/fn_80192410.h" /* fn_80192410 (rule 2: the owner's header) */
#include "ef/eft028_alpha_by_id.h" /* eft028_alpha_by_id (rule 2: the owner's header) */
#include "ef/eft028_capsule_ends.h" /* eft028_capsule_ends (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "gx.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "unsplit/sound.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012BDF4.h"
#define eft_res_spawn_gate_ck fn_800F92F4_hidden_eft_res_h
#include "ef/eft_res.h"
#undef eft_res_spawn_gate_ck
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "ef/eft028_set_scaled.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMatTevColor (rule 2) */
#include "nw4r/g3d/scnmdl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#define eft_res_spawn_gate_ck fn_800F92F4_hidden_eft_res_h
#include "ef/fn_801173AC_types.h"
#undef eft_res_spawn_gate_ck
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define fn_800F92F4_c1 ((s32 (*)(_EFT*, u32))eft_res_spawn_gate_ck)
#define fn_80192410_c1 ((u32 (*)(struct _ENEMY_WORK*))fn_80192410)

/* ---------------------------------------------------------------------------------------------------
 * the per-family work views
 * ------------------------------------------------------------------------------------------------- */

/* The engine's g3d work handle `push_g3d_wk` takes. */
struct _g3d_work;

/* One 0xC-byte eft026 phase slot: its own little fade ramp.  The four colour bytes are read as a word
 * by `fn_800964E4` (the target's own `lwz r0, 0x16(r30)`), so the call sites pun them: they are plain
 * `u8` fields, not a union, because a union would force the field to +0x04. size: 0x0C */
typedef struct _EFT26_PHASE {
    /* +0x00 */ u8 state;      /* 0 = waiting on the timer, 1 = ramping, 2 = done */
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ _GXColor color; /* the material colour the setters take by value (byte-aligned, so it stays at +0x02) */
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ s32 frame;     /* the ramp position, walked against the per-type frame table */
} _EFT26_PHASE;

/* The eft026 work block `eft_res_slot_get(0x8C)` attaches. size: 0x8C */
typedef struct _EFT26_WORK {
    /* +0x00 */ s32 count;             /* pooled models; 1 or 2, from `eft026_model_counts[type]` */
    /* +0x04 */ MHchar* chara[2];      /* the pooled model chars */
    /* +0x0C */ void* models[2];       /* the `res_eft_UV_model_create` results */
    /* +0x14 */ _EFT26_PHASE phase[5]; /* five colour/fade slots */
    /* +0x50 */ VEC3 scale;            /* the per-channel colour scale */
    /* +0x5C */ VEC3 offset;           /* the model offset inside the joint frame */
    /* +0x68 */ u32 joint;             /* the enemy joint the family hangs off */
    /* +0x6C */ u16 rot_a[2];          /* per-model angles fn_80118FF0 advances and applies */
    /* +0x70 */ u16 rot_c;             /* the third angle fn_80118214 applies with rotLocalMatY */
    /* +0x72 */ u8 pad_0x72[2];
    /* +0x74 */ f32 field_0x74;        /* the enemy's height the placement remembers */
    /* +0x78 */ f32 field_0x78;        /* the ramp scale fn_80119450 reads */
    /* +0x7C */ _g3d_work* slots[2][2]; /* the g3d work handles res_eft_UV_model_create fills, two per model */
} _EFT26_WORK; /* size: 0x8C */

/* The enemy-side object `_EFT`'s source_0x30 points at for the eft026 family, seen through the offsets
 * this family touches.  It is the `enemy/ENEMY_WORK.h` record; the offsets this file reads that the
 * shared header does not name yet are kept here (rule 3/4) rather than added to the shared record. */
typedef struct _EFT26_EM {
    /* +0x000 */ u8 pad_0x000[0x03];
    /* +0x003 */ u8 team;          /* em_eft_fx_spawn_free's last argument */
    /* +0x004 */ u8 pad_0x004[0x16 - 0x04];
    /* +0x016 */ u8 area_0x16;     /* copied into `_EFT::area_0x44` */
    /* +0x017 */ u8 pad_0x017[0x40 - 0x17];
    /* +0x040 */ f32 field_0x40;   /* the HP ratio fn_8011870C tests against +0x64 */
    /* +0x044 */ u8 pad_0x044[0x64 - 0x44];
    /* +0x064 */ f32 field_0x64;   /* its maximum */
    /* +0x068 */ u8 pad_0x068[0x13C - 0x68];
    /* +0x13C */ _EFT25_PHYSICS* model_0x13C; /* its `MHchar` sits at +4 */
    /* +0x140 */ u8 pad_0x140[0x18C - 0x140];
    /* +0x18C */ f32 height_0x18C; /* the y the placement measures against */
    /* +0x190 */ u8 pad_0x190[0x1BC - 0x190];
    /* +0x1BC */ u32 rot_x_0x1BC;  /* latched into the record's rotation */
    /* +0x1C0 */ u32 rot_y_0x1C0;
    /* +0x1C4 */ u8 pad_0x1C4[0x1E1 - 0x1C4];
    /* +0x1E1 */ u8 area_no_0x1E1; /* the area the allocator gates on */
} _EFT26_EM; /* size: 0x1E2 (lower bound) */

/* The eft028 work block `eft_res_slot_get(0x48)` attaches (the next unit, `ef/fn_80119C44.c`, owns the
 * record itself). size: 0x18 (lower bound). */
typedef struct _EFT28_WORK {
    /* +0x00 */ s32 mode;
    /* +0x04 */ u8 pad_0x04[4];
    /* +0x08 */ u8 col_r;      /* the tev colour fn_80119AA8/19970 arm */
    /* +0x09 */ u8 col_g;
    /* +0x0A */ u8 col_b;
    /* +0x0B */ u8 col_a;
    /* +0x0C */ u8 param_0x0C[0x10 - 0x0C]; /* pl_capsule_box_set's output */
    /* +0x10 */ f32 field_0x10;             /* eft028_set_scaled's scale */
    /* +0x14 */ u8 key_0x14;                /* eft028_set's colour keys */
    /* +0x15 */ u8 key_0x15;
    /* +0x16 */ u8 key_0x16;
    /* +0x17 */ u8 key_0x17;
} _EFT28_WORK;

/* The 0x1C-byte parameter record `eft_capsule_ctor` builds and `pl_capsule_box_set` reads in eft028_set_capsule. */
typedef struct _EFT28_PARAM {
    /* +0x00 */ VEC3 a;
    /* +0x0C */ VEC3 b;
    /* +0x18 */ f32 c;
} _EFT28_PARAM; /* size: 0x1C */

/* The pooled effect object's vtable slots fn_8011870C/18B2C/18FF0/19450 reach by index. */
typedef void (*EffectVfn0x24)(void* self);
typedef void (*EffectVfn0x28)(void* self, f32 arg);

typedef struct _EFT26_EFFECT_VTBL {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ EffectVfn0x24 vfn_0x24; /* the per-frame effect hook */
    /* +0x28 */ EffectVfn0x28 vfn_0x28; /* the one-float scalar hook */
} _EFT26_EFFECT_VTBL; /* size: 0x2C */

/* The pooled model's virtual interface as these handlers call it: the table slot +0x24 is the per-frame hook and
 * +0x28 the one-float scalar hook; the slots before them are not called here.  Declared only, so this unit emits
 * no table for it.  size: 0x04 */
class EftUvModelHooks {
public:
    virtual void unused_0x08();
    virtual void unused_0x0C();
    virtual void unused_0x10();
    virtual void unused_0x14();
    virtual void unused_0x18();
    virtual void unused_0x1C();
    virtual void unused_0x20();
    virtual void update();
    virtual void set_scalar(f32 value);
};

typedef struct _EFT26_EFFECT {
    /* +0x00 */ _EFT26_EFFECT_VTBL* vtbl;
    /* +0x04 */ u8 pad_0x04[0x2C];
    /* +0x30 */ s32 field_0x30;   /* advanced by 0x100 per frame in fn_8011870C */
} _EFT26_EFFECT; /* size: 0x34 (an approximation; only the two offsets above are read) */

/* The 0x1C-byte record the g3d material accessor callback chain threads through. */
typedef struct _EFT26_MATOBJ {
    /* +0x00 */ u8 pad_0x00[0x1C];
} _EFT26_MATOBJ; /* size: 0x1C (an approximation - only ever passed by pointer) */

/* ---------------------------------------------------------------------------------------------------
 * the runtime records (the shared headers)
 * ------------------------------------------------------------------------------------------------- */

/* The family-26 colour bytes the record carries at +0x14..+0x17 (`ef.h`'s `_EFT` pads them). */
typedef struct _EFT26_RECCOL {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u8 r;
    /* +0x15 */ u8 g;
    /* +0x16 */ u8 b;
    /* +0x17 */ u8 a;
} _EFT26_RECCOL; /* size: 0x18 (lower bound) */

extern "C" {
void eft026_pl_spawn(_EFT26_EM* em, u8 kind, nw4r::math::VEC3* vec, u32 joint, f32 scale);
void fn_80117EFC(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, f32 scale);
_EFT* fn_80117FF8(u8 area, u8 kind);
void fn_80118154(_EFT* self);
void fn_801181D8(_EFT* self);
void fn_80118214(_EFT* self);
void fn_801186C4(_EFT* self);
void fn_8011870C(_EFT* self);
void fn_80118B2C(_EFT* self);
void fn_80118FF0(_EFT* self);
void fn_80119450(_EFT* self);
void fn_80119804(_EFT* self);
void fn_80119814(_EFT* self);
void fn_80119818(MHchar* chr, u8 mode);
void fn_801198F8(MHchar* chr, u8 index);
void eft028_set(u8 kind, u8 variant, u8 id);
void eft028_set_capsule(u8 area);

/* the plain callees with no reconstructed owner yet (declared here, never `extern`-spelled, so the
 * symbol is the target's plain name; the landing pass moves the ones a registered unit owns) */
_EFT* fn_80119C44(u8 kind, u8 variant, u32 arg);
void fn_80119D10(_EFT* self);
void fn_80119D9C(_EFT* self);
/* 0x80041E40 is owned by `src/mh3_pad.cpp`; its header cannot be included here (`ef.h`
 * spells `VEC3_ctor`/`setVec3` differently from `mh3_pad.h`, MWCC (10197)), so this
 * copy stays - normalised to the owner's body (`void*` return).  `vec3_normalize_into`/`addVec3` now
 * come from their owner's header, `fn_8004CAD8.h` (included above, rule 2). */
void vec3_scale_in_place(nw4r::math::VEC3* v, f32 angle);
void mtx34_copy(nw4r::math::MTX34* out, nw4r::math::MTX34* in);
/* eft_res_model_get comes from the owner's header `ef/eft_res.h` (rule 2): this unit's local
 * `void*` copy collided with the owner's `u8*` definition once the header declared it. */

void eft_em_spawn(struct _ENEMY_WORK* em, s32 a, s32 b, nw4r::math::VEC3* v, f32 c);
void pl_capsule_box_set(_EFT28_PARAM* param, void* out);
void eft_capsule_ctor(_EFT28_PARAM* param);
u32 scn_mdl_mat_access_get_tev_color(nw4r::g3d::ScnMdl::CopiedMatAccess* access, u32 arg);
void res_mat_tev_color_copy_ctor(_EFT26_MATOBJ* out, void* in);
void res_mat_tev_color_end_edit(_EFT26_MATOBJ* obj);
void em_eft_fx_spawn_free(s32 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 area, u8 team);

/* The unit's data.  `.sdata`: the six 8-byte alpha key runs ({frame, alpha} pairs, a 0xFF frame ends one).  `.data`:
 * per type (12) the model's effect id, the handler kind, the duration, the spin rate in degrees per frame, the scale
 * key runs ({frame, x, y, z}, a -1 frame ends one), the longer alpha key runs, the per-type alpha key run and the model
 * count. */
u8 eft026_alpha_keys_3[8] = {
    0x00, 0x00, 0x06, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
};
u8 eft026_alpha_keys_kind2[8] = {
    0x00, 0x00, 0x1E, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
};
u8 eft026_alpha_keys_5[8] = {
    0x00, 0x00, 0x04, 0xFF, 0x0F, 0x00, 0xFF, 0x00,
};
u8 eft026_alpha_keys_7[8] = {
    0x00, 0xFF, 0x04, 0xFF, 0x0C, 0x00, 0xFF, 0x00,
};
u8 eft026_alpha_keys_8[8] = {
    0x00, 0x00, 0x0A, 0xBB, 0x14, 0x00, 0xFF, 0x00,
};
u8 eft026_alpha_keys_9[8] = {
    0x00, 0xFF, 0x1E, 0xFF, 0x3C, 0x00, 0xFF, 0x00,
};

u16 eft026_effect_ids[12] = {
    0x001C, 0x002E, 0x002E, 0x004B, 0x001C, 0x001C, 0x001C, 0x001C, 0x00A3, 0x00A3, 0x001C, 0x001C,
};
u8 eft026_handler_kinds[16] = {
    0x00, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
};
s32 eft026_durations[12] = {
    20, 20, 55, 255, 28, 15, 18, 12, 20, 60, 12, 18,
};
f32 eft026_spin_rates[12] = {
    0.0f, 0.0f, 8.0f, 8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f,
};
f32 eft026_scale_keys_2[20] = {
    0.0f, 0.5f, 0.5f, 0.5f,
    5.0f, 1.4f, 1.4f, 0.7f,
    11.0f, 1.3f, 1.3f, 1.0f,
    55.0f, 0.6f, 0.6f, 1.0f,
    -1.0f, 0.6f, 0.6f, 1.0f,
};
f32 eft026_scale_keys_9[16] = {
    0.0f, 0.5f, 0.5f, 0.5f,
    30.0f, 1.4f, 1.4f, 1.0f,
    60.0f, 0.6f, 0.6f, 1.0f,
    -1.0f, 0.6f, 0.6f, 1.0f,
};
u8 eft026_alpha_keys_0[12] = {
    0x00, 0x00, 0x04, 0xFF, 0x0E, 0xFF, 0x14, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
u8 eft026_alpha_keys_1[12] = {
    0x00, 0x00, 0x05, 0xBB, 0x0A, 0xBB, 0x14, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
u8 eft026_alpha_keys_2[12] = {
    0x00, 0x00, 0x08, 0xFF, 0x19, 0xFF, 0x37, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
u8 eft026_alpha_keys_4[12] = {
    0x00, 0x00, 0x04, 0xFF, 0x12, 0xFF, 0x1C, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
u8 eft026_alpha_keys_6[16] = {
    0x00, 0x00, 0x04, 0xFF, 0x05, 0xFF, 0x12, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
u8* eft026_alpha_key_tables[12] = {
    eft026_alpha_keys_0, eft026_alpha_keys_1, eft026_alpha_keys_2, eft026_alpha_keys_3,
    eft026_alpha_keys_4, eft026_alpha_keys_5, eft026_alpha_keys_6, eft026_alpha_keys_7,
    eft026_alpha_keys_8, eft026_alpha_keys_9, eft026_alpha_keys_7, eft026_alpha_keys_6,
};
f32 eft026_scale_keys_b[28] = {
    0.0f, 0.1f, 0.1f, 0.4f,
    8.0f, 1.82f, 1.82f, 0.4f,
    10.0f, 1.8f, 1.8f, 1.0f,
    11.0f, 1.75f, 1.75f, 1.0f,
    14.0f, 1.55f, 1.55f, 1.0f,
    20.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, 1.0f,
};
f32 eft026_scale_keys_a[12] = {
    0.0f, 0.1f, 0.1f, 0.4f,
    30.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, 1.0f,
};
u8 eft026_model_counts[12] = {
    0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
};
}

void rotMatrixY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatZ(u32 angle, nw4r::math::MTX34* mtx);
void getKeyData3(f32* keys, f32 frame, f32* out0, f32* out1, f32* out2);
s32 ran_suu(s32 max);
void eft013_set(_PLW* plw, u8 value);
nw4r::math::VEC3 get_camera_pos();
nw4r::math::VEC3 get_camera_direction();
void get_joint_wmat_em(struct _ENEMY_WORK* em, u32 joint, nw4r::math::MTX34* mtx);
u16 em_get_mot_no(struct _ENEMY_WORK* em);
void push_g3d_wk(struct _g3d_work* work);
void* res_eft_UV_model_create(MHchar* chr, u16 id, u32 a, long b, struct _g3d_work** list, long c, u8 d);

#pragma peephole off
#pragma pool_data off

/* ---------------------------------------------------------------------------------------------------
 * the eft026 setters and its allocator/release/dispatcher
 * ------------------------------------------------------------------------------------------------- */

/* The eft026 setter that arrives mangled in the map: the family's public spawn entry. */
void eft026_set(_PLW* plw, u8 kind, u32 a, u32 b)
{
    _EFT* rec = fn_80117FF8(plw->area_0x16, kind);
    _EFT26_WORK* work;

    if (rec == 0) {
        return;
    }
    rec->source_0x30 = plw;
    rec->field_0x10 = 5;
    rec->rot_0x24.x = plw->param_0x54 + a;
    rec->rot_0x24.y = ((_EFT25_ACTOR*)plw)->angle_0x58;
    work = (_EFT26_WORK*)rec->work_0x38;
    work->joint = 0xff;
    work->rot_c = (u16)b;
    setVector3(&work->offset, 0.0f, 0.0f, 0.0f);
    setVector3(&work->scale, 0.4f, 0.4f, 1.0f);
}

/* The player-side setter: seat the source player at the family allocator, copy its two angle words,
 * the packed vector, the joint and the scale. */
extern "C" void eft026_pl_spawn(_EFT26_EM* em, u8 kind, nw4r::math::VEC3* vec, u32 joint, f32 scale)
{
    _EFT* rec = fn_80117FF8(em->area_no_0x1E1, kind);

    if (rec == 0) {
        return;
    }
    rec->source_0x30 = em;
    rec->rot_0x24.x = em->rot_x_0x1BC;
    rec->rot_0x24.y = em->rot_y_0x1C0;
    rec->rot_0x24.z = 0;
    {
        _EFT26_WORK* work = (_EFT26_WORK*)rec->work_0x38;
        copyVec3(&work->offset, vec);
        work->joint = joint;
        setVector3(&work->scale, scale, scale, scale);
    }
}

/* The position/rotation setter: only the two ramping kinds (7 and 10) take it, every other kind is
 * destroyed again; the record's rot comes from the caller, the position from the vector. */
extern "C" void fn_80117EFC(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, f32 scale)
{
    _EFT* rec = fn_80117FF8(area, kind);
    _EFT26_WORK* work;

    if (rec == 0) {
        return;
    }
    rec->source_0x30 = 0;
    rec->field_0x10 = 0;
    copyVec3(&rec->pos_0x18, pos);
    rec->rot_0x24.x = rot->x;
    rec->rot_0x24.y = rot->y + 0x8000;
    work = (_EFT26_WORK*)rec->work_0x38;
    switch (rec->type_0x02) {
    case 7:
        setVector3(&work->scale, 12.0f, 12.0f, 14.0f);
        break;
    case 10:
        setVector3(&work->scale, 6.0f, 6.0f, 6.0f);
        break;
    default:
        eft_res_slot_release(rec);
        return;
    }
    work->field_0x78 = scale;
}

/* The eft026 allocator: 0x8C-byte work block, the per-type capacity, the two hooks, the family tag,
 * and one pooled model char per slot; the destroy flag depends on the ramping kinds. */
extern "C" _EFT* fn_80117FF8(u8 area, u8 kind)
{
    _EFT* rec;
    _EFT26_WORK* work;
    s32 i;

    if (area != get_now_areano()) {
        return 0;
    }
    rec = (_EFT*)(void*)eft_res_slot_get(0x8c);
    if (rec == 0) {
        return 0;
    }
    rec->type_0x02 = kind;
    rec->release_0x40 = fn_80118154;
    rec->dispatch_0x34 = fn_801181D8;
    work = (_EFT26_WORK*)rec->work_0x38;
    work->count = eft026_model_counts[kind];
    memset(&work->slots[0], 0, 0x10);
    for (i = 0; i < work->count; i++) {
        work->chara[i] = (MHchar*)eft_res_model_get();
        if (work->chara[i] == 0) {
            eft_res_slot_release(rec);
            return 0;
        }
    }
    rec->field_0x03 = 26;
    rec->field_0x04 = 0;
    rec->timer_0x0C = 0;
    rec->field_0x10 = 0;
    rec->area_0x44 = area;
    if ((u32)(rec->type_0x02 - 4) <= 3 || (u32)(rec->type_0x02 - 0xa) <= 1 || (s32)rec->type_0x02 == 0) {
        eft_state_flags_set(rec, 0, 0);
    } else {
        eft_state_flags_set(rec, 1, 0);
    }
    return rec;
}

/* The family release: push the four g3d work slots, hand the model list back and clear the count. */
extern "C" void fn_80118154(_EFT* self)
{
    s32 i;
    s32 j;
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;

    for (i = 0; i < 2; i++) {
        for (j = 1; j >= 0; j--) {
            if (work->slots[i][j] != 0) {
                push_g3d_wk(work->slots[i][j]);
            }
        }
    }
    eft_res_models_release(&work->chara[0], work->count);
    work->count = 0;
}

/* The state dispatcher: 0 places, 1 advances by kind, 2 steps the state, 3 destroys. */
extern "C" void fn_801181D8(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_80118214(self);
    case 1:
        return fn_801186C4(self);
    case 2:
        return fn_80119804(self);
    case 3:
        return fn_80119814(self);
    }
}

/* Pools one model per live char, seeds the five colour phases and places the model by kind (1/2/8/9 hang off the
 * enemy joint, 3 is the two-model break, 7/10 are plain ramps). */
#pragma push
#pragma optimization_level 4
extern "C" void fn_80118214(_EFT* self)
{
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    _EFT26_RECCOL* rec = (_EFT26_RECCOL*)self;
    _EFT26_EM* em;
    nw4r::math::VEC3 vA;
    nw4r::math::VEC3 vB;
    nw4r::math::VEC3 vC;
    nw4r::math::MTX34 mtx;
    s32 i;

    VEC3_ctor(&vA);
    VEC3_ctor(&vB);
    VEC3_ctor(&vC);
    MTX34_ctor(&mtx);
    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        work->models[i] = res_eft_UV_model_create(work->chara[i], eft026_effect_ids[self->type_0x02], 0x118, 0,
                                                  &work->slots[0][0], 1, 0);
        if (work->models[i] == 0) {
            fn_80119814(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    switch (self->type_0x02) {
    case 5:
        work->offset.z = 50.0f;
        /* fall through */
    case 0:
    case 4:
    case 6:
    case 11:
        for (i = 0; i < 5; i++) {
            u32 col;
            work->phase[i].frame = (u16)ran_suu(0) & 3;
            col = get_stg_eft_col(self->area_0x44, 1);
            work->phase[i].color.r = (u8)((col & 0xFF000000) >> 24);
            work->phase[i].color.g = (u8)((col & 0x00FF0000) >> 16);
            work->phase[i].color.b = (u8)((col & 0x0000FF00) >> 8);
            work->phase[i].color.a = 0;
        }
        break;
    case 1:
    case 2:
    case 8:
    case 9:
        switch (self->type_0x02) {
        case 1:
        case 2:
            rec->r = 0x81;
            rec->g = 0x78;
            rec->b = 0;
            rec->a = 4;
            break;
        case 8:
        case 9:
            rec->r = 0x81;
            rec->b = 0;
            rec->a = 5;
            break;
        }
        em = (_EFT26_EM*)self->source_0x30;
        if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        get_joint_wpos_em((struct _ENEMY_WORK*)em, work->joint, &vA);
        copyVec3(&self->pos_0x18, &vA);
        copyVec3(&vB, &work->offset);
        mtx34_identity(&mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        rotLocalMatZ(self->rot_0x24.z, &mtx);
        work->chara[0]->getTevKColor(0, GX_KCOLOR3, &work->phase[0].color);
        work->phase[0].color.a = 0;
        mulVecMat(&vB, &mtx);
        addVec3To(&self->pos_0x18, &vB);
        work->field_0x74 = em->height_0x18C;
        switch (self->type_0x02) {
        case 1:
        case 8: {
            fn_80119818(work->chara[0], 1);
            const nw4r::math::VEC3& camPos = get_camera_pos();
            nw4r::math::VEC3 cam;
            copyVec3(&vC, &camPos);
            subVec3(&cam, &self->pos_0x18, &vC);
            copyVec3(&vA, &cam);
            {
                f32 dist = vec3_len((const f32*)&vA);
                if (dist < 600.0f) {
                    work->chara[0]->setVisibility(2, false);
                    work->chara[0]->setVisibility(4, false);
                } else if (dist < 1200.0f) {
                    work->chara[0]->setVisibility(4, false);
                }
            }
            if ((s32)self->type_0x02 == 8) {
                work->scale.y *= 0.7f;
                work->scale.z *= 1.2f;
            }
            break;
        }
        case 2:
        case 9:
            fn_80119818(work->chara[0], 5);
            break;
        }
        break;
    case 3:
        fn_801198F8(work->chara[0], 1);
        fn_801198F8(work->chara[1], 2);
        rec->r = 0x81;
        rec->g = 0x78;
        rec->b = 0;
        rec->a = 1;
        em = (_EFT26_EM*)self->source_0x30;
        if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        get_joint_wmat_em((struct _ENEMY_WORK*)em, work->joint, &mtx);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        work->chara[0]->getTevKColor(0, GX_KCOLOR3, &work->phase[0].color);
        work->phase[0].color.a = 0;
        copyVec3(&vB, &work->offset);
        mulVecMat(&vB, &mtx);
        addVec3To(&self->pos_0x18, &vB);
        self->field_0x10 = 10;
        break;
    case 7:
    case 10:
        for (i = 0; i < 5; i++) {
            u32 col;
            work->phase[i].frame = 0;
            col = get_stg_eft_col(self->area_0x44, 1);
            work->phase[i].color.r = (u8)((col & 0xFF000000) >> 24);
            work->phase[i].color.g = (u8)((col & 0x00FF0000) >> 16);
            work->phase[i].color.b = (u8)((col & 0x0000FF00) >> 8);
            work->phase[i].color.a = 0;
        }
        ((EftUvModelHooks*)work->models[0])->set_scalar(5.0f);
        break;
    }
    fn_801186C4(self);
}
#pragma pop

/* State 1: dispatch on the per-type handler table. */
extern "C" void fn_801186C4(_EFT* self)
{
    switch (eft026_handler_kinds[self->type_0x02]) {
    case 0:
        return fn_8011870C(self);
    case 1:
        return fn_80118B2C(self);
    case 2:
        return fn_80118FF0(self);
    case 3:
        return fn_80119450(self);
    }
}

/* The kind-0 handler: run the five phase ramps, colour the record through the g3d material access,
 * and seat the models from the enemy joint. */
extern "C" void fn_8011870C(_EFT* self)
{
    _EFT26_EM* em;
    _EFT26_WORK* work;
    nw4r::math::VEC3 vA;
    nw4r::math::VEC3 vB;
    nw4r::math::MTX34 mtx;
    u8 done = 0;
    s32 i;

    VEC3_ctor(&vA);
    VEC3_ctor(&vB);
    MTX34_ctor(&mtx);
    work = (_EFT26_WORK*)self->work_0x38;
    em = (_EFT26_EM*)self->source_0x30;

    if (work->phase[0].state == 2) done = 1;
    if (work->phase[1].state == 2) done++;
    if (work->phase[2].state == 2) done++;
    if (work->phase[3].state == 2) done++;
    if (work->phase[4].state == 2) done++;
    if (fn_800F92F4_c1(self, 0) == 0 || done == 5) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    for (i = 0; i < 5; i++) {
        switch (work->phase[i].state) {
        case 0:
            if (work->phase[i].frame > 0) {
                work->phase[i].frame--;
            } else {
                work->phase[i].frame = 0;
                work->phase[i].state++;
            }
            break;
        case 1:
            work->phase[i].frame++;
            work->phase[i].color.a = eftGetKeyAlpha(eft026_alpha_key_tables[self->type_0x02], work->phase[i].frame);
            if (work->phase[i].frame >= eft026_durations[self->type_0x02]) {
                work->phase[i].state++;
            }
            break;
        }
        {
            nw4r::g3d::ScnMdl::CopiedMatAccess access(
                (nw4r::g3d::ScnMdl*)work->chara[0]->field_0x118, (u32)i);
            if (copied_mat_access_valid_ck(&access) != 0) {
                _EFT26_MATOBJ obj;
                u32 handle = scn_mdl_mat_access_get_tev_color(&access, 0);
                u32 ignored;
                res_mat_tev_color_copy_ctor(&obj, &handle);
                reinterpret_cast<nw4r::g3d::ResMatTevColor*>(&obj)->GXGetTevKColor(GX_KCOLOR3, (GXColor*)&ignored);
                reinterpret_cast<nw4r::g3d::ResMatTevColor*>(&obj)->GXSetTevKColor(GX_KCOLOR3, work->phase[i].color);
                res_mat_tev_color_end_edit(&obj);
            }
        }
    }

    self->field_0x10--;
    switch (self->field_0x06) {
    case 0:
        work->scale.x += 0.12f;
        work->scale.y += 0.12f;
        if (self->field_0x10 < 0) {
            self->field_0x06++;
            self->field_0x10 = 0xe;
        }
        break;
    case 1:
        work->scale.x -= 0.01f;
        work->scale.y -= 0.01f;
        if (self->field_0x10 < 0) {
            self->field_0x06++;
        }
        if ((s32)self->type_0x02 != 5 && (s32)self->type_0x02 != 11 && self->field_0x10 == 0xa) {
            if (em->field_0x40 < em->field_0x64 - 50.0f) {
                eft013_set((_PLW*)em, 4);
            }
        }
        break;
    }

    self->area_0x44 = em->area_0x16;
    em->model_0x13C->chr_0x04.get_joint_wpos(3, &vA);
    copyVec3(&self->pos_0x18, &vA);
    mtx34_identity(&mtx);
    rotLocalMatY(self->rot_0x24.y, &mtx);
    rotLocalMatX(self->rot_0x24.x, &mtx);
    rotLocalMatY(work->rot_c, &mtx);
    rotLocalMatZ(self->rot_0x24.z, &mtx);
    copyVec3(&vA, &work->offset);
    mulVecMat(&vA, &mtx);
    addVec3To(&self->pos_0x18, &vA);
    mtx.m[0][3] = self->pos_0x18.x;
    mtx.m[1][3] = self->pos_0x18.y;
    mtx.m[2][3] = self->pos_0x18.z;
    self->rot_0x24.z = (work->chara[0]->field_0x30 += 0x100);

    for (i = 0; i < work->count; i++) {
        copyVec3(&work->chara[i]->pos_0x04, &self->pos_0x18);
        copyVec3(&work->chara[i]->scale_0x1C, &work->scale);
        work->chara[i]->move2(&mtx, 0);
        ((EftUvModelHooks*)work->models[i])->update();
        eft_res_models_spawn(self, (void**)&work->chara[i], 2, 1, 0);
    }
}

/* The kind-1 handler: run the five ramps with the offset placement and per-channel colour scale. */
#pragma push
#pragma optimization_level 4
extern "C" void fn_80118B2C(_EFT* self)
{
    _EFT26_WORK* work;
    _EFT26_EM* em;
    nw4r::math::VEC3 v0;
    nw4r::math::VEC3 v1;
    nw4r::math::VEC3 v2;
    nw4r::math::VEC3 v3;
    nw4r::math::VEC3 v4;
    nw4r::math::MTX34 mtx;
    s32 i;

    VEC3_ctor(&v0);
    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    VEC3_ctor(&v3);
    VEC3_ctor(&v4);
    MTX34_ctor(&mtx);
    work = (_EFT26_WORK*)self->work_0x38;
    em = (_EFT26_EM*)self->source_0x30;

    if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->timer_0x0C++;
    if (self->timer_0x0C == eft026_durations[self->type_0x02]) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    work->phase[0].color.a = eftGetKeyAlpha(eft026_alpha_key_tables[self->type_0x02], self->timer_0x0C);
    self->area_0x44 = em->area_no_0x1E1;
    self->rot_0x24.z += (u16)(s32)(0.5f + 65536.0f * eft026_spin_rates[self->type_0x02] / 360.0f);

    mtx34_identity(&mtx);
    rotLocalMatY(self->rot_0x24.y, &mtx);
    rotLocalMatX(self->rot_0x24.x, &mtx);
    rotLocalMatZ(self->rot_0x24.z, &mtx);

    switch (self->type_0x02) {
    case 1:
    case 8:
        mtx34_set_trans(&mtx, &self->pos_0x18);
        copyVec3(&v2, &self->pos_0x18);
        copyVec3(&v4, &work->scale);
        break;
    case 2:
    case 9:
    {
        get_joint_wpos_em((struct _ENEMY_WORK*)em, work->joint, &v0);
        self->rot_0x24.z -= 0x444;
        self->pos_0x18.x = v0.x;
        self->pos_0x18.z = v0.z;
        copyVec3(&v1, &work->offset);
        mulVecMat(&v1, &mtx);
        self->pos_0x18.x += v1.x;
        self->pos_0x18.y += em->height_0x18C - work->field_0x74;
        self->pos_0x18.z += v1.z;
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->field_0x74 = em->height_0x18C;
        {
            const nw4r::math::VEC3& camDir = get_camera_direction();
            nw4r::math::VEC3 sum;
            copyVec3(&v3, &camDir);
            vec3_normalize_into(&v3, &v3);
            vec3_scale_in_place(&v3, -200.0f);
            addVec3(&sum, &self->pos_0x18, &v3);
            copyVec3(&v2, &sum);
        }
        self->field_0x10++;
        if (self->field_0x10 > 5) {
            em_eft_fx_spawn_free(6, &self->pos_0x18, &self->rot_0x24, 1.0f, self->area_0x44, em->team);
            self->field_0x10 = 0;
        }
        switch (self->type_0x02) {
        case 2:
            getKeyData3(eft026_scale_keys_2, (f32)self->timer_0x0C, &v4.x, &v4.y, &v4.z);
            break;
        case 9:
            getKeyData3(eft026_scale_keys_9, (f32)self->timer_0x0C, &v4.x, &v4.y, &v4.z);
            break;
        }
        v4.x *= work->scale.x;
        v4.y *= work->scale.y;
        v4.z *= work->scale.z;
        break;
    }
    }

    {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        work->phase[0].color.r = (col & 0xFF000000) >> 24;
        work->phase[0].color.g = (col & 0x00FF0000) >> 16;
        work->phase[0].color.b = (col & 0x0000FF00) >> 8;
    }
    switch (self->type_0x02) {
    case 1:
    case 2:
        for (i = 0; i < 4; i++) {
            work->chara[0]->setTevKColor(i, GX_KCOLOR3, &work->phase[0].color);
        }
        break;
    case 8:
    case 9:
        for (i = 0; i < 5; i++) {
            work->chara[0]->setTevKColor(i, GX_KCOLOR3, &work->phase[0].color);
        }
        break;
    }

    for (i = 0; i < work->count; i++) {
        copyVec3(&work->chara[i]->scale_0x1C, &v4);
        work->chara[i]->move2(&mtx, 0);
        ((EftUvModelHooks*)work->models[i])->update();
        if (get_camera_pos().y < 0.0f) {
            eft_res_models_spawn(self, (void**)&work->chara[i], 2, 1, &v2);
        }
    }
}
#pragma pop

/* The kind-2 handler: a two-stage timer with a wake-up step and the matrix placement. */
extern "C" void fn_80118FF0(_EFT* self)
{
    _EFT26_WORK* work;
    _EFT26_EM* em;
    nw4r::math::VEC3 v0;
    nw4r::math::VEC3 v1;
    nw4r::math::VEC3 v2;
    nw4r::math::VEC3 v3;
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    s32 i;

    VEC3_ctor(&v0);
    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    VEC3_ctor(&v3);
    MTX34_ctor(&mtxA);
    MTX34_ctor(&mtxB);
    work = (_EFT26_WORK*)self->work_0x38;
    em = (_EFT26_EM*)self->source_0x30;

    if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    {
        u16 mot = em_get_mot_no((struct _ENEMY_WORK*)em);
        if ((u32)(mot - 0xd4) <= 3) {
            if ((u32)(mot - 0xd4) <= 1 || mot == 0xd7) {
                if (self->field_0x06 == 0) {
                    self->timer_0x0C++;
                }
            }
        } else {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    if (fn_80192410_c1((struct _ENEMY_WORK*)em) == 1 && self->field_0x06 == 0) {
        self->timer_0x0C = 0x1e;
        self->field_0x06 = 1;
    }
    if (self->field_0x06 != 0) {
        self->timer_0x0C--;
    }
    if (self->timer_0x0C < 0 && self->field_0x06 != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->field_0x06 == 0) {
        self->field_0x10--;
        if (self->field_0x10 < 0) {
            setVector3(&v1, 0.0f, -80.0f, 120.0f);
            eft_em_spawn((struct _ENEMY_WORK*)em, 0x1e, 0xf, &v1, 1.0f);
            self->field_0x10 = 3;
        }
    }
    self->area_0x44 = em->area_no_0x1E1;
    get_joint_wmat_em((struct _ENEMY_WORK*)em, work->joint, &mtxA);
    copyVec3(&v1, &work->offset);
    mulVecMat(&v1, &mtxA);
    mtx34_trans_add(&mtxA, &v1);
    mtx34_trans_get(&mtxA, &self->pos_0x18);
    work->rot_a[0] += (u16)(s32)(0.5f + 65536.0f * eft026_spin_rates[self->type_0x02] / 360.0f);
    work->rot_a[1] -= (u16)(s32)(0.5f + 65536.0f * eft026_spin_rates[self->type_0x02] / 360.0f);

    if (self->field_0x06 != 0) {
        getKeyData3(eft026_scale_keys_a, (f32)self->timer_0x0C, &v3.x, &v3.y, &v3.z);
        work->phase[0].color.a = eftGetKeyAlpha(eft026_alpha_keys_kind2, self->timer_0x0C);
    } else {
        getKeyData3(eft026_scale_keys_b, (f32)self->timer_0x0C, &v3.x, &v3.y, &v3.z);
        work->phase[0].color.a = eftGetKeyAlpha(eft026_alpha_keys_3, self->timer_0x0C);
    }
    v3.x *= work->scale.x;
    v3.y *= work->scale.y;
    v3.z *= work->scale.z;
    {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        work->phase[0].color.r = (col & 0xFF000000) >> 24;
        work->phase[0].color.g = (col & 0x00FF0000) >> 16;
        work->phase[0].color.b = (col & 0x0000FF00) >> 8;
    }
    rotLocalMatX(0xe39, &mtxA);

    for (i = 0; i < work->count; i++) {
        mtx34_copy(&mtxB, &mtxA);
        rotLocalMatZ(work->rot_a[i], &mtxB);
        work->chara[i]->move2(&mtxB, 0);
        copyVec3(&work->chara[i]->scale_0x1C, &v3);
        ((EftUvModelHooks*)work->models[i])->update();
        work->chara[i]->setTevKColor(0, GX_KCOLOR3, &work->phase[0].color);
        if (get_camera_pos().y < 0.0f) {
            eft_res_models_spawn(self, (void**)&work->chara[i], 2, 1, 0);
        }
    }
}

/* The kind-3 handler: the same ramp/placement but with the vertical offset and the clamped step. */
extern "C" void fn_80119450(_EFT* self)
{
    s32 i;
    u8 done = 0;
    nw4r::math::VEC3 v0;
    nw4r::math::VEC3 v1;
    nw4r::math::VEC3 v2;
    nw4r::math::MTX34 mtx;
    _EFT26_WORK* work;

    VEC3_ctor(&v0);
    VEC3_ctor(&v1);
    MTX34_ctor(&mtx);
    work = (_EFT26_WORK*)self->work_0x38;

    for (i = 0; i < 5; i++) {
        switch (work->phase[i].state) {
        case 0:
            if (work->phase[i].frame > 0) {
                work->phase[i].frame--;
            } else {
                work->phase[i].frame = 0;
                work->phase[i].state++;
            }
            break;
        case 1:
            work->phase[i].frame++;
            work->phase[i].color.a = eftGetKeyAlpha(eft026_alpha_key_tables[self->type_0x02], work->phase[i].frame);
            if (work->phase[i].frame >= (s32)eft026_durations[self->type_0x02]) {
                work->phase[i].state++;
            }
            break;
        }
        {
            nw4r::g3d::ScnMdl::CopiedMatAccess access(
                (nw4r::g3d::ScnMdl*)work->chara[0]->field_0x118, (u32)i);
            if (copied_mat_access_valid_ck(&access) != 0) {
                _EFT26_MATOBJ obj;
                u32 handle = scn_mdl_mat_access_get_tev_color(&access, 0);
                res_mat_tev_color_copy_ctor(&obj, &handle);
                reinterpret_cast<nw4r::g3d::ResMatTevColor*>(&obj)->GXSetTevKColor(GX_KCOLOR3, work->phase[i].color);
                res_mat_tev_color_end_edit(&obj);
            }
        }
    }

    {
        if (work->phase[0].state == 2) done = 1;
        if (work->phase[1].state == 2) done++;
        if (work->phase[2].state == 2) done++;
        if (work->phase[3].state == 2) done++;
        if (work->phase[4].state == 2) done++;
        if (done == 5) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    setVector3(&v0, 0.0f, 0.0f,
               work->field_0x78 * ((f32)self->field_0x10 / (f32)eft026_durations[self->type_0x02]));
    switch (self->type_0x02) {
    case 7:
        v0.z -= 5000.0f;
        break;
    case 10:
        v0.z -= 500.0f;
        break;
    }
    vec3_scale_in_place(&v0, -1.0f);
    self->field_0x10++;
    if (self->field_0x10 > 4) {
        self->field_0x10 = 4;
    }

    mtx34_identity(&mtx);
    rotMatrixY(self->rot_0x24.y, &mtx);
    rotLocalMatX(self->rot_0x24.x, &mtx);
    rotLocalMatZ(self->rot_0x24.z, &mtx);
    mulVecMat(&v0, &mtx);
    addVec3(&v2, &self->pos_0x18, &v0);
    copyVec3(&v1, &v2);
    mtx.m[0][3] = v1.x;
    mtx.m[1][3] = v1.y;
    mtx.m[2][3] = v1.z;
    self->rot_0x24.z = (work->chara[0]->field_0x30 += 0x100);

    for (i = 0; i < work->count; i++) {
        copyVec3(&work->chara[i]->pos_0x04, &v1);
        copyVec3(&work->chara[i]->scale_0x1C, &work->scale);
        work->chara[i]->move2(&mtx, 0);
        ((EftUvModelHooks*)work->models[i])->update();
        eft_res_models_spawn(self, (void**)&work->chara[i], 2, 1, 0);
    }
}

/* State 2/3 and the two visibility helpers the placement calls. */
extern "C" void fn_80119804(_EFT* self)
{
    self->state_0x05++;
}

extern "C" void fn_80119814(_EFT* self)
{
    eft_res_slot_release(self);
}

extern "C" void fn_80119818(MHchar* chr, u8 mode)
{
    s32 i;

    switch (mode) {
    case 1:
        for (i = mode; i < 5; i++) {
            chr->setVisibility(i, true);
        }
        for (i = 5; i <= 8; i++) {
            chr->setVisibility(i, false);
        }
        break;
    case 5:
        for (i = mode; i <= 8; i++) {
            chr->setVisibility(i, true);
        }
        for (i = 1; i < mode; i++) {
            chr->setVisibility(i, false);
        }
        break;
    }
}

extern "C" void fn_801198F8(MHchar* chr, u8 index)
{
    s32 i;

    for (i = 1; i < 3; i++) {
        if (i == index) {
            chr->setVisibility(i, true);
        } else {
            chr->setVisibility(i, false);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * the eft028 (break/crumble) setters
 * ------------------------------------------------------------------------------------------------- */

extern "C" void eft028_set(u8 kind, u8 variant, u8 id)
{
    u32 mode;
    _EFT* rec;
    _EFT28_WORK* work;

    switch (variant) {
    case 0:
        mode = 3;
        break;
    case 1:
        mode = 1;
        break;
    default:
        return;
    }
    rec = fn_80119C44(kind, id, mode);
    if (rec == 0) {
        return;
    }
    work = (_EFT28_WORK*)rec->work_0x38;
    work->key_0x14 = 0xff;
    work->key_0x15 = 0xff;
    work->key_0x16 = 0xff;
    if (get_now_mapno() == 0) {
        work->key_0x17 = eft028_alpha_by_id[id];
    } else {
        work->key_0x17 = 0xff;
    }
    rec->timer_0x0C = 0;
}

void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 variant, long timer)
{
    _EFT* rec = fn_80119C44(kind, variant, 1);

    if (rec == 0) {
        return;
    }
    copyVec3(&rec->pos_0x18, pos);
    rec->timer_0x0C = timer;
}

extern "C" void eft028_set_capsule(u8 area)
{
    _EFT* rec;
    _EFT28_WORK* work;
    _EFT28_PARAM param;

    eft_capsule_ctor(&param);
    if (area != get_now_areano()) {
        return;
    }
    rec = (_EFT*)(void*)eft_res_slot_get(0x48);
    if (rec == 0) {
        return;
    }
    rec->field_0x03 = 28;
    rec->type_0x02 = 2;
    rec->area_0x44 = area;
    eft_state_flags_set(rec, 0, 0);
    work = (_EFT28_WORK*)rec->work_0x38;
    work->mode = 1;
    copyVec3(&param.a, &eft028_capsule_ends[0]);
    copyVec3(&param.b, &eft028_capsule_ends[1]);
    param.c = 410.0f;
    pl_capsule_box_set(&param, &work->param_0x0C[0]);
    work->col_r = 0xff;
    work->col_g = 0xff;
    work->col_b = 0xff;
    work->col_a = 0;
    copyVec3(&rec->pos_0x18, &param.a);
    rec->timer_0x0C = 0;
    rec->release_0x40 = fn_80119D10;
    rec->dispatch_0x34 = fn_80119D9C;
}

extern "C" void eft028_set_scaled(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 variant, long timer)
{
    _EFT* rec = fn_80119C44(kind, variant, 1);
    _EFT28_WORK* work;

    if (rec == 0) {
        return;
    }
    work = (_EFT28_WORK*)rec->work_0x38;
    work->field_0x10 = scale;
    copyVec3(&rec->pos_0x18, pos);
    eft_rot_vec_copy(&rec->rot_0x24, rot);
    rec->timer_0x0C = timer;
}
