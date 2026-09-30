/* ef/fn_801173AC.cpp - the `.text` 0x801173AC..0x80119C44 run (30 functions), the tail of the eft024
 * job machine plus the whole eft025 player family, the whole eft026 enemy family and the head of eft028.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_
 * name this file uses is a bare .text entry in config/RMHE08/symbols.txt); the runtime dump resolves only
 * the two family setters `eft026_set` (0x80117DA8) and `eft028_set_koware` (0x80119A40).
 *
 * Registration (docs/plan.md 12).  Class 2/3 evidence: the runtime dump's own `eft026_set` /
 * `eft028_set_koware` are defined inside the range, so the module is `ef`; the range is NOT one TU (it
 * holds the eft024 tail, all of eft025, all of eft026 and the eft028 head), so no single family name
 * covers it and class 4 applies - the file keeps the map's `fn_801173AC` stem, the scheme the bracketing
 * `ef/fn_8011722C.c` and `ef/fn_80119C44.c` units use.  C++: the range's own definitions
 * `eft026_set__FP4_PLWUcUlUl` and `eft028_set_koware__FUcPQ34nw4r4math4VEC3Ucl` are manglings, so the
 * file is `.cpp` and every plain definition is `extern "C"` so its emitted name stays the map's stem
 * (playbook row 42).  Sections: extab 0x8000C49C..0x8000C554, extabindex 0x800265D4..0x800266E8,
 * `.text` 0x801173AC..0x80119C44 - the runs the two neighbouring units leave.
 *
 * What it is.
 *   * fn_801173AC is the eft024 job machine's kind-1 state-1 handler (the sibling of
 *     `ef/fn_8011722C.c`'s kind-1 state-0): it integrates the job's spin deltas into the rotation,
 *     pushes the resulting transform onto the pooled `MHchar` and advances the job when the model
 *     falls below the ground.
 *   * fn_80117688..fn_80117DA4 are the whole `eft025` player family: two setters (the second carries a
 *     scale), the allocator that stamps the family tag 25 and installs the two hooks, the release, the
 *     state dispatcher and its four state handlers.
 *   * eft026_set, fn_80117E58 and fn_80117EFC are the three `eft026` setters (the enemy-fold family's
 *     spawn path - horse/boat style folding), fn_80117FF8 is its allocator (family tag 26), fn_80118154
 *     its release, and fn_801181D8/fn_80118214/fn_801186C4/fn_8011870C/fn_80118B2C/fn_80118FF0/
 *     fn_80119450/fn_80119804/fn_80119814/fn_80119818/fn_801198F8 its state machine and helpers.
 *   * fn_80119970, eft028_set_koware, fn_80119AA8 and fn_80119BB0 are the `eft028` (break/crumble)
 *     family's setters; they build their records through the next unit's `fn_80119C44`.
 *
 * Language notes.  `get_camera_pos`/`get_camera_direction` are declared with C++ linkage at the global
 * scope (their map names are `__Fv` and the SDK returns by value through sret - the same finding as
 * `ef/effect.cpp`).  `rotMatrixX/Y`, `rotLocalMatX/Y/Z`, `get_joint_wmat_em`, `em_get_mot_no`,
 * `res_eft_UV_model_create`, `getKeyData3`, `ran_suu` and `msl::`-free `push_g3d_wk` are mangled in the
 * target, so they are declared at C++ scope (rule 9: the call never spells the mangling).
 *
 * Types.  `_EFT`, `_PLW` and `MHchar` come from `ef.h`/`pl.h`; the agent 3d `ScnMdl::CopiedMatAccess`
 * from `nw4r/g3d/scnmdl.h`.  The per-family work blocks are unit-local views (`_EFT24_CHARA`,
 * `_EFT25_WORK`, `_EFT26_WORK`, `_EFT26_EM`, `_EFT28_WORK`) because each family reads a different
 * subset at the same offsets; the `MHchar`/`Effect` vtable entries are reached through the
 * function-pointer tables the target's own `lwz r12, off(r12)` shape requires (the `ef/ef_creationqueue`
 * convention - a real `virtual` cannot be declared without re-emitting the class's `.data` vtable).
 *
 * Data.  The unit owns no pool section (the target object carries none): the shared key tables,
 * id/frame/rate tables, jump table and the `.sdata2` scalars are declared by their map names and never
 * defined (playbook 29).
 *
 * Result (this round).  All 30 symbols are >= 80 % and 15 are byte-identical; the unit measures
 * 94.01193 % fuzzy over the real split object (`.text` 10380 B against the target's 10392, extab 184 B
 * and extabindex 276 B - both the target's exact sizes).  Residuals, all measured, none a source shape:
 *   fn_80118154 87.67  the four `slots[]` pushes pair; the pointer walk `_g3d_work** p =
 *                      (_g3d_work**)&work->slots[i*2+1]` with `p[0]`/`p--` is best (the indexed
 *                      `slots[i*2+j]` form is 87.09), the last 4 B being retail's own base+offset
 *                      induction (`r31 = work; addi r30,r31,4; addi r31,r31,8`)
 *   fn_80118214 89.89  the 12-way switch and its `jumptable_805A06BC` pair; residual is the placement
 *                      tail's register pressure
 *   fn_801173AC 91.30, fn_80118FF0 91.01, fn_80119AA8 91.86, fn_80118B2C 92.09, fn_80119450 92.13,
 *   fn_8011870C 93.59, fn_80117FF8 94.08, fn_80119818 95.71, fn_801198F8 96.0, fn_80117688 97.69,
 *   fn_80117760 97.77, fn_80117894 97.96, fn_80117A1C 98.82 - scheduling / frame layout only.
 *
 * Load-bearing source shapes (each measured; the wrong form costs real points):
 *   * `#pragma peephole off` is required file-wide (playbook 39, the same finding as
 *     `ef/fn_80114E34.cpp`): with the pass on MWCC fuses the nested slot loop's `subi`/`cmpwi` into
 *     `subic.` and compresses the frame, costing fn_80118B2C 20 points, fn_80119970 10 and
 *     eft028_set_koware 14.
 *   * `_EFT26_PHASE`'s colour must be four plain `u8` fields, not a union: a union is alignment 4,
 *     which moves it to +0x04 and pushes `_EFT26_WORK::slots` from +0x7C to +0x90 (the target's own
 *     `lwz r0,0x16(r30)` reads the four bytes as a word, so the two `fn_800964E4` call sites pun
 *     `(u32*)&phase[i].color_r`).
 *   * `lbl_805A04B0` is `s32[]`, not `u32[]`: the eft026 timer tests compile to the target's signed
 *     `cmpw` only with the signed view.
 *   * `get_camera_direction()` is called mid-case, after the position accumulation - the target's own
 *     instruction order (fn_80118B2C).
 *   * the eft026 jump table is the compiler's own switch table (data stays out of the split object), so
 *     `splits.txt` claims only `.text`/extab/extabindex.
 */

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
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "nw4r/g3d/scnmdl.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* The retail object keeps the raw record-form chains and the `subi rX,rY,1` / `cmpwi` pair the peephole
 * pass fuses into `subic.`; `gekko`/MWCC's default here is peephole ON (the ef band's cflags_main).  The
 * predecessor unit `ef/fn_80114E34.cpp` needs the same (playbook row 39).  File scope, one unit. */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * the per-family work views
 * ------------------------------------------------------------------------------------------------- */

/* The engine's g3d work handle `push_g3d_wk` takes. */
struct _g3d_work;

/* The eft024 job's kind-1 work view (`fn_80114E34.cpp`'s family-24 `job->work_0x38` block, 0x84 B).
 * `ef/fn_8011722C.c` is the kind-1 state-0 sibling and names the same offsets. size: 0x20 (lower bound,
 * the highest offset this file reads). */
typedef struct _EFT24_CHARA {
    /* +0x00 */ s32 count;        /* live character handles, always 1 in the kind-1 slot */
    /* +0x04 */ MHchar* chara[1]; /* the pooled handles */
    /* +0x08 */ VEC3 pos;         /* per-frame position delta, integrated by fn_801173AC */
    /* +0x14 */ u16 rot_x;        /* rotation angles pushed to the model */
    /* +0x16 */ u16 rot_y;
    /* +0x18 */ u16 rot_z;
    /* +0x1A */ s16 spin_x;       /* per-frame spin deltas, decayed then integrated */
    /* +0x1C */ s16 spin_y;
    /* +0x1E */ s16 spin_z;
} _EFT24_CHARA;

/* The player body sub-object `_PLW.physics_0x13C` points at: its `MHchar` sits at +4 (the view
 * `ef/fn_80114E34.cpp` states for the same offset). size: 0x144 (lower bound). */
typedef struct _EFT25_PHYSICS {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ MHchar chr_0x04;
} _EFT25_PHYSICS;

/* The player record as the eft025/eft026 family reads it.  It is `pl.h`'s `_PLW` at the same offsets;
 * this view exists because the shared record still spells +0x0C and +0x58 `unkNN` and rule 7 forbids
 * those identifiers in a `src/` file. size: 0x140 (lower bound). */
typedef struct _EFT25_ACTOR {
    /* +0x000 */ u8 pad_0x000[0x0a];
    /* +0x00A */ u8 field_0x00A;         /* the mode the state-1 gate tests against 10 */
    /* +0x00B */ u8 pad_0x00B[0x0c - 0x0b];
    /* +0x00C */ u16 motion_0x00C;       /* the motion index the gate excludes above 2 */
    /* +0x00E */ u8 pad_0x00E[0x54 - 0x0e];
    /* +0x054 */ u32 angle_0x54;         /* the base angle eft026_set adds its first word to */
    /* +0x058 */ u32 angle_0x58;         /* the y rotation angle (0x4000 is a quarter turn) */
    /* +0x05C */ u8 pad_0x05C[0x13c - 0x5c];
    /* +0x13C */ _EFT25_PHYSICS* physics_0x13C;
} _EFT25_ACTOR;

/* The eft025 kind-3 source: its `MHchar` sits at +8 (the view `ef/fn_80114E34.cpp` states for the same
 * record). size: 0x148 (lower bound). */
typedef struct _EFT25_CHARA_SRC {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ MHchar chr_0x08;
} _EFT25_CHARA_SRC;

/* The eft025 work block `fn_800F8788(0x10)` attaches. size: 0x10 */
typedef struct _EFT25_WORK {
    /* +0x00 */ s32 count;        /* pooled models, always 1 */
    /* +0x04 */ void* models[1];  /* the pooled `nw4r::ef::Effect` handles */
    /* +0x08 */ s32 motion;       /* the source's motion number latched in state 0, type 2 */
    /* +0x0C */ f32 scale;        /* the per-record scale fn_801176F0 carries */
} _EFT25_WORK;

/* One 0xC-byte eft026 phase slot: its own little fade ramp.  The four colour bytes are read as a word
 * by `fn_800964E4` (the target's own `lwz r0, 0x16(r30)`), so the call sites pun them: they are plain
 * `u8` fields, not a union, because a union would force the field to +0x04. size: 0x0C */
typedef struct _EFT26_PHASE {
    /* +0x00 */ u8 state;      /* 0 = waiting on the timer, 1 = ramping, 2 = done */
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 color_r;    /* read as a _GXColor by the material setters */
    /* +0x03 */ u8 color_g;
    /* +0x04 */ u8 color_b;
    /* +0x05 */ u8 color_a;
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ s32 frame;     /* the ramp position, walked against the per-type frame table */
} _EFT26_PHASE;

/* The eft026 work block `fn_800F8788(0x8C)` attaches. size: 0x8C */
typedef struct _EFT26_WORK {
    /* +0x00 */ s32 count;             /* pooled models; 1 or 2, from `lbl_805A06B0[type]` */
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
    /* +0x7C */ void* slots[4];        /* the g3d work handles res_eft_UV_model_create fills */
} _EFT26_WORK; /* size: 0x8C */

/* The enemy-side object `_EFT`'s source_0x30 points at for the eft026 family, seen through the offsets
 * this family touches.  It is the `enemy/ENEMY_WORK.h` record; the offsets this file reads that the
 * shared header does not name yet are kept here (rule 3/4) rather than added to the shared record. */
typedef struct _EFT26_EM {
    /* +0x000 */ u8 pad_0x000[0x03];
    /* +0x003 */ u8 team;          /* fn_8011D7B0's last argument */
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

/* The eft028 work block `fn_800F8788(0x48)` attaches (the next unit, `ef/fn_80119C44.c`, owns the
 * record itself). size: 0x18 (lower bound). */
typedef struct _EFT28_WORK {
    /* +0x00 */ s32 mode;
    /* +0x04 */ u8 pad_0x04[4];
    /* +0x08 */ u8 col_r;      /* the tev colour fn_80119AA8/19970 arm */
    /* +0x09 */ u8 col_g;
    /* +0x0A */ u8 col_b;
    /* +0x0B */ u8 col_a;
    /* +0x0C */ u8 param_0x0C[0x10 - 0x0C]; /* fn_8028F558's output */
    /* +0x10 */ f32 field_0x10;             /* fn_80119BB0's scale */
    /* +0x14 */ u8 key_0x14;                /* fn_80119970's colour keys */
    /* +0x15 */ u8 key_0x15;
    /* +0x16 */ u8 key_0x16;
    /* +0x17 */ u8 key_0x17;
} _EFT28_WORK;

/* The 0x1C-byte parameter record `fn_800FA3B8` builds and `fn_8028F558` reads in fn_80119AA8. */
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

/* ---------------------------------------------------------------------------------------------------
 * the unit's own prototypes, then the foreign callees
 * ------------------------------------------------------------------------------------------------- */

extern "C" {

/* this unit's own symbols (definitions below; the map's plain stems) */
void fn_801173AC(_EFT* self);
void fn_80117688(_PLW* plw, u8 kind);
void fn_801176F0(_PLW* plw, f32 value);
_EFT* fn_80117760(u8 kind, u8 area);
void fn_8011781C(_EFT* self);
void fn_80117858(_EFT* self);
void fn_80117894(_EFT* self);
void fn_80117A1C(_EFT* self);
void fn_80117D94(_EFT* self);
void fn_80117DA4(_EFT* self);
void fn_80117E58(_EFT26_EM* em, u8 kind, nw4r::math::VEC3* vec, u32 joint, f32 scale);
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
void fn_80119970(u8 kind, u8 variant, u8 id);
_EFT* fn_80119AA8(u8 area);
void fn_80119BB0(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 variant, long timer);

/* the plain callees with no reconstructed owner yet (declared here, never `extern`-spelled, so the
 * symbol is the target's plain name; the landing pass moves the ones a registered unit owns) */
_EFT* fn_80119C44(u8 kind, u8 variant, u32 arg);
void fn_80119D10(_EFT* self);
void fn_80119D9C(_EFT* self);
/* 0x80041E40 is owned by `src/mh3_pad.cpp`; its header cannot be included here (`include/ef.h`
 * spells `VEC3_ctor`/`setVec3` differently from `include/mh3_pad.h`, MWCC (10197)), so this
 * copy stays - normalised to the owner's body (`void*` return).  `fn_80050850`/`addVec3` now
 * come from their owner's header, `include/fn_8004CAD8.h` (included above, rule 2). */
void fn_800513F0(nw4r::math::VEC3* v, f32 angle);
void fn_800532DC(nw4r::math::MTX34* out, nw4r::math::MTX34* in);
/* fn_800F8914 comes from the owner's header `ef/eft_res.h` (rule 2): this unit's local
 * `void*` copy collided with the owner's `u8*` definition once the header declared it. */
s32 fn_800F92F4(_EFT* self, u32 flag);
void fn_800E0A14(void* mhchar, u32 joint, Mtx34* out);
u32 event_demo_ck(void);
u32 fn_80192410(struct _ENEMY_WORK* em);
void eft_em_spawn(struct _ENEMY_WORK* em, s32 a, s32 b, nw4r::math::VEC3* v, f32 c);
void fn_8028F558(_EFT28_PARAM* param, void* out);
void fn_800FA3B8(_EFT28_PARAM* param);
u32 fn_8007BE2C(nw4r::g3d::ScnMdl::CopiedMatAccess* access, u32 arg);
void fn_8006F0E8(_EFT26_MATOBJ* out, void* in);
void fn_800963C0(_EFT26_MATOBJ* obj, u32 a, void* out);
void fn_800964E4(_EFT26_MATOBJ* obj, u32 a, void* in);
void fn_8006F0DC(_EFT26_MATOBJ* obj);
void fn_8011D7B0(s32 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 area, u8 team);

/* the shared data the range reads (declared, never defined here) */
extern u16 lbl_805A0488[];
extern u8 lbl_805A04A0[];
extern s32 lbl_805A04B0[];
extern f32 lbl_805A04E0[];
extern u8* lbl_805A05E0[];
extern f32 lbl_805A0510[];
extern f32 lbl_805A0560[];
extern f32 lbl_805A0610[];
extern f32 lbl_805A0680[];
extern u8 lbl_805A06B0[];
extern u32 jumptable_805A06BC[];
extern u8 lbl_805A0730[];
extern u16 lbl_80791938[];
extern u8 lbl_80791940[];
extern u8 lbl_80791948[];
extern nw4r::math::VEC3 lbl_806A4548[];
extern f32 lbl_80796A88;
extern f32 lbl_80796A8C;
extern f32 lbl_80796A90;
extern f32 lbl_80796A94;
extern f32 lbl_80796A98;
extern f32 lbl_80796A9C;
extern f32 lbl_80796AA0;
extern f64 lbl_80796AA8;
extern f32 lbl_80796AB0;
extern f32 lbl_80796AB4;
extern f32 lbl_80796AB8;
extern f32 lbl_80796ABC;
extern f32 lbl_80796AC0;
extern f32 lbl_80796AC4;
extern f32 lbl_80796AC8;
extern f32 lbl_80796ACC;
extern f32 lbl_80796AD0;
extern f32 lbl_80796AD4;
extern f32 lbl_80796AD8;
extern f32 lbl_80796ADC;
extern f32 lbl_80796AE0;
extern f32 lbl_80796AE4;
extern f32 lbl_80796AE8;
extern f32 lbl_80796AEC;
extern f32 lbl_80796AF0;
extern f32 lbl_80796AF4;
extern f32 lbl_80796AF8;
extern f32 lbl_80796AFC;
extern f32 lbl_80796B00;
extern f32 lbl_80796B04;
extern f64 lbl_80796B08;
extern f32 lbl_80796B10;
extern f32 lbl_80796B14;
extern f32 lbl_80796B18;
extern f32 lbl_80796B1C;
extern f32 lbl_80796B20;
extern f32 lbl_80796B28;
}

/* the mangled callees: declared at C++ scope so the call reaches the target's mangled name (rule 9) */
void rotMatrixX(u32 angle, nw4r::math::MTX34* mtx);
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

/* ---------------------------------------------------------------------------------------------------
 * fn_801173AC - the eft024 kind-1 state-1 handler
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_801173AC(_EFT* self)
{
    _EFT24_CHARA* work = (_EFT24_CHARA*)self->work_0x38;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 pos;

    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);

    work->spin_x = (s16)((f32)work->spin_x * lbl_80796A88);
    work->spin_y = (s16)((f32)work->spin_y * lbl_80796A8C);
    work->spin_z = (s16)((f32)work->spin_z * lbl_80796A90);
    work->spin_x = (s16)(work->spin_x + (s16)((((u16)ran_suu(1) & 0x1FF) - 0x1FF) >> 2));
    work->spin_y = (s16)(work->spin_y + (s16)((((u16)ran_suu(1) & 0xFF) - 0xFF) >> 2));
    work->spin_z = (s16)(work->spin_z + (s16)((((u16)ran_suu(1) & 0xFF) - 0xFF) >> 2));
    work->rot_x = (u16)(work->rot_x + (u16)work->spin_x);
    work->rot_y = (u16)(work->rot_y + (u16)work->spin_y);
    work->rot_z = (u16)(work->rot_z + (u16)work->spin_z);

    mtx34_identity(&mtx);
    rotMatrixX(work->rot_x, &mtx);
    rotLocalMatY(work->rot_y, &mtx);
    rotLocalMatZ(work->rot_z, &mtx);

    work->pos.x += lbl_80796A94 * mtx.m[1][0];
    work->pos.y += lbl_80796A94 * mtx.m[1][1];
    work->pos.z += lbl_80796A94 * mtx.m[1][2];
    work->pos.x *= lbl_80796A98;
    work->pos.y *= lbl_80796A98;
    work->pos.z *= lbl_80796A9C;

    copyVec3(&pos, &work->pos);
    mulVecMat(&pos, &mtx);
    addVec3To(&work->chara[0]->pos_0x04, &pos);
    work->chara[0]->field_0x28 = work->rot_x;
    work->chara[0]->field_0x2C = work->rot_y;
    work->chara[0]->field_0x30 = work->rot_z;

    if (work->chara[0]->pos_0x04.y < lbl_80796AA0) {
        self->state_0x05++;
    } else {
        s32 i;
        for (i = 0; i < work->count; i++) {
            work->chara[i]->move(0);
        }
        fn_800F93D8(self, (void**)&work->chara[0], 2, work->count, 0);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * the eft025 player family (0x80117688..0x80117DA4)
 * ------------------------------------------------------------------------------------------------- */

/* The two spawn setters: they gate on the caller record's area and hand the family allocator the
 * kind, then remember the caller as the record's source. */
extern "C" void fn_80117688(_PLW* plw, u8 kind)
{
    u8 area = plw->area_0x16;
    _EFT* rec;

    if (area != get_now_areano()) {
        return;
    }
    rec = fn_80117760(kind, area);
    if (rec != 0) {
        rec->source_0x30 = plw;
    }
}

extern "C" void fn_801176F0(_PLW* plw, f32 value)
{
    u8 area = plw->effect_key_0x1A4;
    _EFT* rec;

    if (area != get_now_areano()) {
        return;
    }
    rec = fn_80117760(3, area);
    if (rec != 0) {
        rec->source_0x30 = plw;
        ((_EFT25_WORK*)rec->work_0x38)->scale = value;
    }
}

/* The family allocator: one 0x10-byte model slot, the family tag 25, one live handle, and the two
 * hooks that travel with the record. */
extern "C" _EFT* fn_80117760(u8 kind, u8 area)
{
    _EFT* rec;

    if (area != get_now_areano()) {
        return 0;
    }
    rec = (_EFT*)(void*)fn_800F8788(0x10);
    if (rec == 0) {
        return 0;
    }
    ((_EFT25_WORK*)rec->work_0x38)->count = 1;
    rec->field_0x03 = 25;
    rec->type_0x02 = kind;
    rec->area_0x44 = area;
    fn_800F9DF4(rec, 1, 0);
    rec->release_0x40 = fn_8011781C;
    rec->dispatch_0x34 = fn_80117858;
    return rec;
}

/* The family release: hands the pooled handles back and clears the count. */
extern "C" void fn_8011781C(_EFT* self)
{
    _EFT25_WORK* work = (_EFT25_WORK*)self->work_0x38;

    push_eft_effect_heap_num((nw4r::ef::Effect**)&work->models[0], work->count);
    work->count = 0;
}

/* The state dispatcher: state 0 creates and places, 1 advances, 2 steps the state and 3 destroys. */
extern "C" void fn_80117858(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        return fn_80117894(self);
    case 1:
        return fn_80117A1C(self);
    case 2:
        return fn_80117D94(self);
    case 3:
        return fn_80117DA4(self);
    }
}

/* State 0: pool the effect, then seat the record at the source's placement joint (kind 1) or latch
 * the source's motion number (kind 2), and hand the state machine on to state 1. */
extern "C" void fn_80117894(_EFT* self)
{
    _EFT25_WORK* work = (_EFT25_WORK*)self->work_0x38;
    nw4r::math::VEC3 vec;
    nw4r::math::MTX34 mtx;

    VEC3_ctor(&vec);
    MTX34_ctor(&mtx);
    self->state_0x05++;
    work->models[0] = res_eft_create(lbl_80791938[self->type_0x02], 0x16, 0);
    if (work->models[0] == 0) {
        fn_80117DA4(self);
        return;
    }
    self->flag_0x01 = 1;
    switch (self->type_0x02) {
    case 1: {
        _PLW* actor = (_PLW*)self->source_0x30;
        if (fn_800F92F4(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        fn_800E0A14(&((_EFT25_PHYSICS*)actor->physics_0x13C)->chr_0x04, 0xb, &mtx);
        setVector3(&vec, lbl_80796AB0, lbl_80796AB4, lbl_80796AB8);
        mulVecMat(&vec, &mtx);
        self->pos_0x18.x = mtx.m[0][3] + vec.x;
        self->pos_0x18.y = mtx.m[1][3] + vec.y;
        self->pos_0x18.z = mtx.m[2][3] + vec.z;
        break;
    }
    case 2: {
        _PLW* actor = (_PLW*)self->source_0x30;
        if (fn_800F92F4(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        work->motion = (u16)Get_motion_no(actor);
        break;
    }
    }
    fn_80117A1C(self);
}

/* State 1: place the pooled handles each frame.  Type 0 seats them at the source's joint-11 matrix,
 * type 1 seats them at the source position (both then push the world position), type 2 re-seats them
 * against the source's joint-17 matrix and its per-frame parameter scale, and type 3 places them from
 * the source's joint-16 matrix with the record scale.  Then every live handle is moved and the record
 * advances to state 2 once they all die. */
extern "C" void fn_80117A1C(_EFT* self)
{
    _EFT25_WORK* work = (_EFT25_WORK*)self->work_0x38;
    nw4r::math::VEC3 vec;
    nw4r::math::VEC3 vel;
    nw4r::math::MTX34 mtxA;
    nw4r::math::MTX34 mtxB;
    s32 i;

    VEC3_ctor(&vec);
    VEC3_ctor(&vel);
    MTX34_ctor(&mtxA);
    MTX34_ctor(&mtxB);

    if (fn_800F92F4(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (event_demo_ck() == 1) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    switch (self->type_0x02) {
    case 0:
        fn_800E0A14(&((_EFT25_PHYSICS*)((_PLW*)self->source_0x30)->physics_0x13C)->chr_0x04, 0xb, &mtxA);
        setVector3(&vec, lbl_80796AB0, lbl_80796AB4, lbl_80796AB8);
        mulVecMat(&vec, &mtxA);
        self->pos_0x18.x = mtxA.m[0][3] + vec.x;
        self->pos_0x18.y = mtxA.m[1][3] + vec.y;
        self->pos_0x18.z = mtxA.m[2][3] + vec.z;
        /* fall through */
    case 1:
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans((nw4r::ef::Effect*)work->models[i], &self->pos_0x18);
        }
        break;
    case 2: {
        _EFT25_ACTOR* actor = (_EFT25_ACTOR*)self->source_0x30;
        if (actor->field_0x00A != 0xa || actor->motion_0x00C <= 2 ||
            work->motion != (u16)Get_motion_no((_PLW*)actor)) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        self->field_0x10 = actor->angle_0x58 + 0x4000;
        fn_800E0A14(&actor->physics_0x13C->chr_0x04, 0xb, &mtxA);
        setVector3(&vec, lbl_80796AB0, lbl_80796AB4, lbl_80796AB8);
        mulVecMat(&vec, &mtxA);
        mtx34_identity(&mtxB);
        rotLocalMatY(self->field_0x10, &mtxB);
        mtxB.m[0][3] = mtxA.m[0][3] + vec.x;
        self->pos_0x18.x = mtxA.m[0][3] + vec.x;
        mtxB.m[1][3] = mtxA.m[1][3] + vec.y;
        self->pos_0x18.y = mtxA.m[1][3] + vec.y;
        mtxB.m[2][3] = mtxA.m[2][3] + vec.z;
        self->pos_0x18.z = mtxA.m[2][3] + vec.z;
        for (i = 0; i < work->count; i++) {
            ((nw4r::ef::Effect*)work->models[i])->SetRootMtx(mtxB);
        }
        break;
    }
    case 3:
        fn_800E0A14(&((_EFT25_CHARA_SRC*)self->source_0x30)->chr_0x08, 0x10, &mtxA);
        setVector3(&vec, lbl_80796AB4, lbl_80796AB8, lbl_80796ABC);
        mulVecMat(&vec, &mtxA);
        self->pos_0x18.x = mtxA.m[0][3] + vec.x;
        self->pos_0x18.y = mtxA.m[1][3] + vec.y;
        self->pos_0x18.z = mtxA.m[2][3] + vec.z;
        change_paramscale_eff((nw4r::ef::Effect*)work->models[0], work->scale);
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans((nw4r::ef::Effect*)work->models[i], &self->pos_0x18);
        }
        break;
    }

    for (i = 0; i < work->count; i++) {
        if (effect_move((nw4r::ef::Effect*)work->models[i]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    fn_800F93D8(self, (void**)&work->models[0], 1, work->count, 0);
}

/* State 2 and state 3: advance the state, then destroy the record. */
extern "C" void fn_80117D94(_EFT* self)
{
    self->state_0x05++;
}

extern "C" void fn_80117DA4(_EFT* self)
{
    fn_800F886C(self);
}

/* ---------------------------------------------------------------------------------------------------
 * the eft026 setters and its allocator/release/dispatcher
 * ------------------------------------------------------------------------------------------------- */

/* The player-side setter: seat the source player at the family allocator, copy its two angle words,
 * the packed vector, the joint and the scale. */
extern "C" void fn_80117E58(_EFT26_EM* em, u8 kind, nw4r::math::VEC3* vec, u32 joint, f32 scale)
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
        setVector3(&work->scale, lbl_80796ACC, lbl_80796ACC, lbl_80796AD0);
        break;
    case 10:
        setVector3(&work->scale, lbl_80796AD4, lbl_80796AD4, lbl_80796AD4);
        break;
    default:
        fn_800F886C(rec);
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
    rec = (_EFT*)(void*)fn_800F8788(0x8c);
    if (rec == 0) {
        return 0;
    }
    rec->type_0x02 = kind;
    rec->release_0x40 = fn_80118154;
    rec->dispatch_0x34 = fn_801181D8;
    work = (_EFT26_WORK*)rec->work_0x38;
    work->count = lbl_805A06B0[kind];
    memset(&work->slots[0], 0, 0x10);
    for (i = 0; i < work->count; i++) {
        work->chara[i] = (MHchar*)fn_800F8914();
        if (work->chara[i] == 0) {
            fn_800F886C(rec);
            return 0;
        }
    }
    rec->field_0x03 = 26;
    rec->field_0x04 = 0;
    rec->timer_0x0C = 0;
    rec->field_0x10 = 0;
    rec->area_0x44 = area;
    if ((u8)((u8)kind - 4) <= 3 || (u8)((u8)kind - 0xa) <= 1 || kind == 0) {
        fn_800F9DF4(rec, 0, 0);
    } else {
        fn_800F9DF4(rec, 1, 0);
    }
    return rec;
}

/* The family release: push the four g3d work slots, hand the model list back and clear the count. */
extern "C" void fn_80118154(_EFT* self)
{
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    s32 i;
    s32 j;

    for (i = 0; i < 2; i++) {
        _g3d_work** p = (_g3d_work**)&work->slots[i * 2 + 1];
        for (j = 1; j >= 0; j--) {
            if (p[0] != 0) {
                push_g3d_wk(p[0]);
            }
            p--;
        }
    }
    fn_800F8A44(&work->chara[0], work->count);
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

/* State 0: pool one model per live char, then seed the five colour phases and place the model by
 * kind.  Kinds 1/2/8/9 are the folded-body kinds (they hang off the enemy joint and ramp a colour),
 * 3 is the two-model break, 7/10 are the plain ramps, and 0/4/5/6/11 only seed the phases. */
extern "C" void fn_80118214(_EFT* self)
{
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    _EFT26_RECCOL* rec = (_EFT26_RECCOL*)self;
    _EFT26_EM* em;
    nw4r::math::VEC3 vA;
    nw4r::math::VEC3 vB;
    nw4r::math::VEC3 vC;
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 cam;
    s32 i;

    VEC3_ctor(&vA);
    VEC3_ctor(&vB);
    VEC3_ctor(&vC);
    MTX34_ctor(&mtx);
    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        work->models[i] = res_eft_UV_model_create(work->chara[i], lbl_805A0488[self->type_0x02], 0x118, 0,
                                                  (struct _g3d_work**)&work->slots[0], 1, 0);
        if (work->models[i] == 0) {
            fn_80119814(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    switch (self->type_0x02) {
    case 5:
        work->offset.z = lbl_80796AD8;
        /* fall through */
    case 0:
    case 4:
    case 6:
    case 11:
        for (i = 0; i < 5; i++) {
            u32 col;
            work->phase[i].frame = (u16)ran_suu(0) & 3;
            col = get_stg_eft_col(self->area_0x44, 1);
            work->phase[i].color_r = (u8)(col >> 24);
            work->phase[i].color_g = (u8)(col >> 16);
            work->phase[i].color_b = (u8)(col >> 8);
            work->phase[i].color_a = 0;
        }
        break;
    case 1:
    case 2:
    case 8:
    case 9:
        if (self->type_0x02 == 1 || self->type_0x02 == 2) {
            rec->r = 0x81;
            rec->g = 0x78;
            rec->b = 0;
            rec->a = 4;
        } else {
            rec->r = 0x81;
            rec->b = 0;
            rec->a = 5;
        }
        em = (_EFT26_EM*)self->source_0x30;
        if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        get_joint_wpos_em((struct _ENEMY_WORK*)em, work->joint, &vA);
        copyVec3(&self->pos_0x18, &vA);
        copyVec3(&vB, &work->offset);
        mtx34_identity(&mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        rotLocalMatZ(self->rot_0x24.z, &mtx);
        work->chara[0]->getTevKColor(0, GX_KCOLOR3, (GXColor*)&work->phase[0].color_r);
        work->phase[0].color_a = 0;
        mulVecMat(&vB, &mtx);
        addVec3To(&self->pos_0x18, &vB);
        work->field_0x74 = em->height_0x18C;
        if (self->type_0x02 == 1 || self->type_0x02 == 8) {
            nw4r::math::VEC3 camPos = get_camera_pos();
            fn_80119818(work->chara[0], 1);
            copyVec3(&vC, &camPos);
            subVec3(&cam, &self->pos_0x18, &vC);
            copyVec3(&vA, &cam);
            {
                f32 dist = fn_80050F24((const f32*)&vA);
                if (dist < lbl_80796ADC) {
                    work->chara[0]->setVisibility(2, false);
                    work->chara[0]->setVisibility(4, false);
                } else if (dist < lbl_80796AE0) {
                    work->chara[0]->setVisibility(4, false);
                }
            }
            if (self->type_0x02 == 8) {
                work->scale.y *= lbl_80796AE4;
                work->scale.z *= lbl_80796AE8;
            }
        } else {
            fn_80119818(work->chara[0], 5);
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
            self->state_0x05++;
            return;
        }
        get_joint_wmat_em((struct _ENEMY_WORK*)em, work->joint, &mtx);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        work->chara[0]->getTevKColor(0, GX_KCOLOR3, (GXColor*)&work->phase[0].color_r);
        work->phase[0].color_a = 0;
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
            work->phase[i].color_r = (u8)(col >> 24);
            work->phase[i].color_g = (u8)(col >> 16);
            work->phase[i].color_b = (u8)(col >> 8);
            work->phase[i].color_a = 0;
        }
        ((_EFT26_EFFECT*)work->models[0])->vtbl->vfn_0x28(work->models[0], lbl_80796AEC);
        break;
    }
    fn_801186C4(self);
}

/* State 1: dispatch on the per-type handler table. */
extern "C" void fn_801186C4(_EFT* self)
{
    switch (lbl_805A04A0[self->type_0x02]) {
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
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    _EFT26_EM* em = (_EFT26_EM*)self->source_0x30;
    nw4r::math::VEC3 vA;
    nw4r::math::VEC3 vB;
    nw4r::math::MTX34 mtx;
    u32 done = 0;
    s32 i;

    VEC3_ctor(&vA);
    VEC3_ctor(&vB);
    MTX34_ctor(&mtx);

    if (work->phase[0].state == 2) done = 1;
    if (work->phase[1].state == 2) done++;
    if (work->phase[2].state == 2) done++;
    if (work->phase[3].state == 2) done++;
    if (work->phase[4].state == 2) done++;
    if (fn_800F92F4(self, 0) == 0 || done != 5) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    for (i = 0; i < 5; i++) {
        _EFT26_PHASE* p = &work->phase[i];
        switch (p->state) {
        case 0:
            if (p->frame > 0) {
                p->frame--;
            } else {
                p->frame = 0;
                p->state++;
            }
            break;
        case 1:
            p->frame++;
            p->color_a = eftGetKeyAlpha((u8*)lbl_805A05E0[self->type_0x02], p->frame);
            if (p->frame >= lbl_805A04B0[self->type_0x02]) {
                p->state++;
            }
            break;
        }
        {
            nw4r::g3d::ScnMdl::CopiedMatAccess access(
                (nw4r::g3d::ScnMdl*)work->chara[0]->field_0x118, (u32)i);
            if (fn_800E2994(&access) != 0) {
                _EFT26_MATOBJ obj;
                u32 handle = fn_8007BE2C(&access, 0);
                u32 ignored;
                fn_8006F0E8(&obj, &handle);
                fn_800963C0(&obj, 3, &ignored);
                fn_800964E4(&obj, 3, (u32*)&p->color_r);
                fn_8006F0DC(&obj);
            }
        }
    }

    self->field_0x10--;
    switch (self->field_0x06) {
    case 0:
        work->scale.x += lbl_80796AF0;
        work->scale.y += lbl_80796AF0;
        if (self->field_0x10 < 0) {
            self->field_0x06++;
            self->field_0x10 = 0xe;
        }
        break;
    case 1:
        work->scale.x -= lbl_80796AF4;
        work->scale.y -= lbl_80796AF4;
        if (self->field_0x10 < 0) {
            self->field_0x06++;
        }
        if (self->type_0x02 != 5 && self->type_0x02 != 11 && self->field_0x10 == 0xa) {
            if (em->field_0x40 < em->field_0x64 - lbl_80796AD8) {
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
        ((_EFT26_EFFECT*)work->models[i])->vtbl->vfn_0x24(work->models[i]);
        fn_800F93D8(self, (void**)&work->chara[i], 2, 1, 0);
    }
}

/* The kind-1 handler: run the five ramps with the offset placement and per-channel colour scale. */
extern "C" void fn_80118B2C(_EFT* self)
{
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    _EFT26_EM* em = (_EFT26_EM*)self->source_0x30;
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

    if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->timer_0x0C++;
    if (self->timer_0x0C == lbl_805A04B0[self->type_0x02]) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    work->phase[0].color_a = eftGetKeyAlpha((u8*)lbl_805A05E0[self->type_0x02], self->timer_0x0C);
    self->area_0x44 = em->area_no_0x1E1;
    self->rot_0x24.z += (u16)(s32)(lbl_80796AF8 + lbl_80796AFC * lbl_805A04E0[self->type_0x02] / lbl_80796B00);

    mtx34_identity(&mtx);
    rotLocalMatY(self->rot_0x24.y, &mtx);
    rotLocalMatX(self->rot_0x24.x, &mtx);
    rotLocalMatZ(self->rot_0x24.z, &mtx);

    switch (self->type_0x02) {
    case 1:
    case 8:
        fn_800FBB90(&mtx, &self->pos_0x18);
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
        fn_800FBB90(&mtx, &self->pos_0x18);
        work->field_0x74 = em->height_0x18C;
        {
            nw4r::math::VEC3 camDir = get_camera_direction();
            copyVec3(&v3, &camDir);
        }
        fn_80050850(&v3, &v3);
        fn_800513F0(&v3, lbl_80796B04);
        addVec3(&v0, &self->pos_0x18, &v3);
        copyVec3(&v2, &v0);
        self->field_0x10++;
        if (self->field_0x10 > 5) {
            fn_8011D7B0(6, &self->pos_0x18, &self->rot_0x24, lbl_80796AC8, self->area_0x44, em->team);
            self->field_0x10 = 0;
        }
        switch (self->type_0x02) {
        case 2:
            getKeyData3(lbl_805A0510, (f32)self->timer_0x0C, &v4.x, &v4.y, &v4.z);
            break;
        case 9:
            getKeyData3(lbl_805A0560, (f32)self->timer_0x0C, &v4.x, &v4.y, &v4.z);
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
        work->phase[0].color_r = (u8)(col >> 24);
        work->phase[0].color_g = (u8)(col >> 16);
        work->phase[0].color_b = (u8)(col >> 8);
    }
    if (self->type_0x02 == 1 || self->type_0x02 == 2) {
        for (i = 0; i < 4; i++) {
            work->chara[0]->setTevKColor(i, GX_KCOLOR3, (GXColor*)&work->phase[0].color_r);
        }
    } else if (self->type_0x02 == 8 || self->type_0x02 == 9) {
        for (i = 0; i < 5; i++) {
            work->chara[0]->setTevKColor(i, GX_KCOLOR3, (GXColor*)&work->phase[0].color_r);
        }
    }

    {
        f32 zero = lbl_80796AC0;
        for (i = 0; i < work->count; i++) {
            copyVec3(&work->chara[i]->scale_0x1C, &v4);
            work->chara[i]->move2(&mtx, 0);
            ((_EFT26_EFFECT*)work->models[i])->vtbl->vfn_0x24(work->models[i]);
            if (get_camera_pos().y < zero) {
                fn_800F93D8(self, (void**)&work->chara[i], 2, 1, &v2);
            }
        }
    }
}

/* The kind-2 handler: a two-stage timer with a wake-up step and the matrix placement. */
extern "C" void fn_80118FF0(_EFT* self)
{
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    _EFT26_EM* em = (_EFT26_EM*)self->source_0x30;
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

    if (em_work_die_ck((struct _ENEMY_WORK*)em) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    {
        u16 mot = em_get_mot_no((struct _ENEMY_WORK*)em);
        if ((u16)(mot - 0xd4) <= 3) {
            if ((u16)(mot - 0xd4) <= 1 || mot == 0xd7) {
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
    if (fn_80192410((struct _ENEMY_WORK*)em) == 1 && self->field_0x06 == 0) {
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
            setVector3(&v1, lbl_80796AC0, lbl_80796B10, lbl_80796B14);
            eft_em_spawn((struct _ENEMY_WORK*)em, 0x1e, 0xf, &v1, lbl_80796AC8);
            self->field_0x10 = 3;
        }
    }
    self->area_0x44 = em->area_no_0x1E1;
    get_joint_wmat_em((struct _ENEMY_WORK*)em, work->joint, &mtxA);
    copyVec3(&v1, &work->offset);
    mulVecMat(&v1, &mtxA);
    mtx34_trans_add(&mtxA, &v1);
    mtx34_trans_get(&mtxA, &self->pos_0x18);
    work->rot_a[0] += (u16)(s32)(lbl_80796AF8 + lbl_80796AFC * lbl_805A04E0[self->type_0x02] / lbl_80796B00);
    work->rot_a[1] -= (u16)(s32)(lbl_80796AF8 + lbl_80796AFC * lbl_805A04E0[self->type_0x02] / lbl_80796B00);

    if (self->field_0x06 != 0) {
        getKeyData3(lbl_805A0680, (f32)self->timer_0x0C, &v3.x, &v3.y, &v3.z);
        work->phase[0].color_a = eftGetKeyAlpha(lbl_80791948, self->timer_0x0C);
    } else {
        getKeyData3(lbl_805A0610, (f32)self->timer_0x0C, &v3.x, &v3.y, &v3.z);
        work->phase[0].color_a = eftGetKeyAlpha(lbl_80791940, self->timer_0x0C);
    }
    v3.x *= work->scale.x;
    v3.y *= work->scale.y;
    v3.z *= work->scale.z;
    {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        work->phase[0].color_r = (u8)(col >> 24);
        work->phase[0].color_g = (u8)(col >> 16);
        work->phase[0].color_b = (u8)(col >> 8);
    }
    rotLocalMatX(0xe39, &mtxA);

    for (i = 0; i < work->count; i++) {
        fn_800532DC(&mtxB, &mtxA);
        rotLocalMatZ(work->rot_a[i], &mtxB);
        copyVec3(&work->chara[i]->scale_0x1C, &v3);
        work->chara[i]->move2(&mtxB, 0);
        ((_EFT26_EFFECT*)work->models[i])->vtbl->vfn_0x24(work->models[i]);
        work->chara[i]->setTevKColor(0, GX_KCOLOR3, (GXColor*)&work->phase[0].color_r);
        if (get_camera_pos().y < lbl_80796AC0) {
            fn_800F93D8(self, (void**)&work->chara[i], 2, 1, 0);
        }
    }
}

/* The kind-3 handler: the same ramp/placement but with the vertical offset and the clamped step. */
extern "C" void fn_80119450(_EFT* self)
{
    _EFT26_WORK* work = (_EFT26_WORK*)self->work_0x38;
    nw4r::math::VEC3 v0;
    nw4r::math::VEC3 v1;
    nw4r::math::VEC3 v2;
    nw4r::math::MTX34 mtx;
    s32 i;

    VEC3_ctor(&v0);
    VEC3_ctor(&v1);
    MTX34_ctor(&mtx);

    for (i = 0; i < 5; i++) {
        _EFT26_PHASE* p = &work->phase[i];
        switch (p->state) {
        case 0:
            if (p->frame > 0) {
                p->frame--;
            } else {
                p->frame = 0;
                p->state++;
            }
            break;
        case 1:
            p->frame++;
            p->color_a = eftGetKeyAlpha((u8*)lbl_805A05E0[self->type_0x02], p->frame);
            if (p->frame >= (s32)lbl_805A04B0[self->type_0x02]) {
                p->state++;
            }
            break;
        }
        {
            nw4r::g3d::ScnMdl::CopiedMatAccess access(
                (nw4r::g3d::ScnMdl*)work->chara[0]->field_0x118, (u32)i);
            if (fn_800E2994(&access) != 0) {
                _EFT26_MATOBJ obj;
                u32 handle = fn_8007BE2C(&access, 0);
                fn_8006F0E8(&obj, &handle);
                fn_800964E4(&obj, 3, (u32*)&p->color_r);
                fn_8006F0DC(&obj);
            }
        }
    }

    {
        u32 done = 0;
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
    setVector3(&v0, lbl_80796AC0, lbl_80796AC0,
               work->field_0x78 * ((f32)self->field_0x10 / (f32)lbl_805A04B0[self->type_0x02]));
    if (self->type_0x02 == 7) {
        v0.z -= lbl_80796B18;
    } else if (self->type_0x02 == 10) {
        v0.z -= lbl_80796B1C;
    }
    fn_800513F0(&v0, lbl_80796B20);
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
        ((_EFT26_EFFECT*)work->models[i])->vtbl->vfn_0x24(work->models[i]);
        fn_800F93D8(self, (void**)&work->chara[i], 2, 1, 0);
    }
}

/* State 2/3 and the two visibility helpers the placement calls. */
extern "C" void fn_80119804(_EFT* self)
{
    self->state_0x05++;
}

extern "C" void fn_80119814(_EFT* self)
{
    fn_800F886C(self);
}

extern "C" void fn_80119818(MHchar* chr, u8 mode)
{
    u32 i;

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
        for (i = 1; i < (u32)mode; i++) {
            chr->setVisibility(i, false);
        }
        break;
    }
}

extern "C" void fn_801198F8(MHchar* chr, u8 index)
{
    u32 i;

    for (i = 1; i < 3; i++) {
        if (i == (u32)index) {
            chr->setVisibility(i, true);
        } else {
            chr->setVisibility(i, false);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------
 * the eft028 (break/crumble) setters
 * ------------------------------------------------------------------------------------------------- */

/* The two eft026 setters that arrive mangled in the map: they are the family's public spawn entries. */
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
    setVector3(&work->offset, lbl_80796AC0, lbl_80796AC0, lbl_80796AC0);
    setVector3(&work->scale, lbl_80796AC4, lbl_80796AC4, lbl_80796AC8);
}

extern "C" void fn_80119970(u8 kind, u8 variant, u8 id)
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
        work->key_0x17 = lbl_805A0730[id];
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

extern "C" _EFT* fn_80119AA8(u8 area)
{
    _EFT* rec;
    _EFT28_WORK* work;
    _EFT28_PARAM param;

    fn_800FA3B8(&param);
    if (area != get_now_areano()) {
        return 0;
    }
    rec = (_EFT*)(void*)fn_800F8788(0x48);
    if (rec == 0) {
        return 0;
    }
    rec->field_0x03 = 28;
    rec->type_0x02 = 2;
    rec->area_0x44 = area;
    fn_800F9DF4(rec, 0, 0);
    work = (_EFT28_WORK*)rec->work_0x38;
    work->mode = 1;
    copyVec3(&param.a, &lbl_806A4548[0]);
    copyVec3(&param.b, &lbl_806A4548[1]);
    param.c = lbl_80796B28;
    fn_8028F558(&param, &work->param_0x0C[0]);
    work->col_r = 0xff;
    work->col_g = 0xff;
    work->col_b = 0xff;
    work->col_a = 0;
    copyVec3(&rec->pos_0x18, &param.a);
    rec->timer_0x0C = 0;
    rec->release_0x40 = fn_80119D10;
    rec->dispatch_0x34 = fn_80119D9C;
    return rec;
}

extern "C" void fn_80119BB0(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 variant, long timer)
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


