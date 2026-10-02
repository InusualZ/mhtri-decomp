/* ef/fn_801173AC.cpp - the effect band at 0x801173AC
 *
 * `.text` 0x801173AC..0x80117DA8, 10 functions written (the rest of the range is not decompiled yet).
 * Phase 4 (docs/splits/phase4): recut registered unit; the functions of the neighbouring units were cut out of this file.
 * Each function keeps the `#pragma` state it had in its retired source. The retired header's notes follow below.
 */

/* Retired header of `ef/fn_801173AC.cpp` (kept for its notes and residuals): */
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
#include "ef/fn_801173AC_types.h"

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

/* The eft025 kind-3 source: its `MHchar` sits at +8 (the view `ef/fn_80114E34.cpp` states for the same
 * record). size: 0x148 (lower bound). */
typedef struct _EFT25_CHARA_SRC {
    /* +0x00 */ u8 pad_0x00[8];
    /* +0x08 */ MHchar chr_0x08;
} _EFT25_CHARA_SRC;

/* The eft025 work block `eft_res_slot_get(0x10)` attaches. size: 0x10 */
typedef struct _EFT25_WORK {
    /* +0x00 */ s32 count;        /* pooled models, always 1 */
    /* +0x04 */ void* models[1];  /* the pooled `nw4r::ef::Effect` handles */
    /* +0x08 */ s32 motion;       /* the source's motion number latched in state 0, type 2 */
    /* +0x0C */ f32 scale;        /* the per-record scale fn_801176F0 carries */
} _EFT25_WORK;

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

/* eft_res_model_get comes from the owner's header `ef/eft_res.h` (rule 2): this unit's local
 * `void*` copy collided with the owner's `u8*` definition once the header declared it. */
s32 eft_res_spawn_gate_ck(_EFT* self, u32 flag);
void fn_800E0A14(void* mhchar, u32 joint, Mtx34* out);
u32 event_demo_ck(void);

extern u16 lbl_80791938[];

extern f32 lbl_80796A88;
extern f32 lbl_80796A8C;
extern f32 lbl_80796A90;
extern f32 lbl_80796A94;
extern f32 lbl_80796A98;
extern f32 lbl_80796A9C;
extern f32 lbl_80796AA0;

extern f32 lbl_80796AB0;
extern f32 lbl_80796AB4;
extern f32 lbl_80796AB8;
extern f32 lbl_80796ABC;
}

/* the mangled callees: declared at C++ scope so the call reaches the target's mangled name (rule 9) */
void rotMatrixX(u32 angle, nw4r::math::MTX34* mtx);

void rotLocalMatY(u32 angle, nw4r::math::MTX34* mtx);
void rotLocalMatZ(u32 angle, nw4r::math::MTX34* mtx);

s32 ran_suu(s32 max);

#pragma peephole off

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
        eft_res_models_spawn(self, (void**)&work->chara[0], 2, work->count, 0);
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
    rec = (_EFT*)(void*)eft_res_slot_get(0x10);
    if (rec == 0) {
        return 0;
    }
    ((_EFT25_WORK*)rec->work_0x38)->count = 1;
    rec->field_0x03 = 25;
    rec->type_0x02 = kind;
    rec->area_0x44 = area;
    eft_state_flags_set(rec, 1, 0);
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
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
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
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
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

    if (eft_res_spawn_gate_ck(self, 0) == 0) {
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
    eft_res_models_spawn(self, (void**)&work->models[0], 1, work->count, 0);
}

/* State 2 and state 3: advance the state, then destroy the record. */
extern "C" void fn_80117D94(_EFT* self)
{
    self->state_0x05++;
}

extern "C" void fn_80117DA4(_EFT* self)
{
    eft_res_slot_release(self);
}
