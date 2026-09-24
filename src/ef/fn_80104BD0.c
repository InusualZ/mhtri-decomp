/* auto/80104BD0_fn_80104BD0.c - one function, .text 0x80104BD0..0x80105314.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * Per-frame handler of the enemy hit-effect controller.  It reads the enemy work object and the
 * effect work block off the effect object, spawns the two `nw4r::ef` effects the type selects, keeps
 * the second one alive only for the four "paired" types, drives each effect's root matrix from the
 * enemy joint (or from the enemy's own position), colours them from the stage effect-colour table and
 * scales them per frame.
 *
 * The object is the generic 72-byte effect object the `eft002` cluster also uses (`_EFT` in
 * `800FCED4_fn_800FCED4.cpp`): flag at +0x01, type at +0x02, state at +0x05, position at +0x18, the
 * enemy work at +0x30, the per-family work block at +0x38 and the area byte at +0x44.  Only this
 * family reads +0x24 (a `_CP_VECTOR` rotation source for `cpSetRotMatrix`) and its work block is the
 * larger `_EM_EFT_WORK` (count, joint, two effects, an RGBA and a scale).
 *
 * The two big `switch`es are the type dispatch: the first (joint bound) picks one of three
 * colour/height pairs, the second (no joint) picks one of two colours.  Their case sets are read off
 * the target's comparison trees and each is enumerated in full because MWCC groups consecutive cases
 * into the same run (a value that is not a case would fall to `default`, so every value in a
 * non-default run must be written out).
 *
 * Callees with a C++-mangled map name are declared with that spelling, as `800D7F54`/`803066F0` do:
 * the file is compiled `-lang=c` (see the brief), so the identifier is emitted verbatim and the
 * relocation pairs with the map's symbol.
 *
 * Result: `fn_80104BD0` 100 %, `.text` (0x744), `extab` (0x8) and `extabindex` (0xC) byte-identical to
 * the target.
 *
 * Load-bearing source shapes (each one measured; the wrong form costs real points):
 *   * the two big `switch`es enumerate **every** value of each non-default run.  MWCC merges
 *     consecutive same-body cases into a run, so a value left out falls to `default` and the
 *     comparison tree changes.  The target's tree gives the runs: the joint-bound switch has
 *     `[0x1E,0x20] += 3`, `0x37 += 1`, `[0x39,0x3C] += 0xC/+8`, `[0x4E,0x50] += 0x34`,
 *     `[0x70,0x73] += 4`, and the colour switches are `[0x00,0x02]`/`[0x04,0x0B]`/`[0x55,0x73]`/...
 *     vs the default colour (the full lists are in the bodies).
 *   * the file needs the peephole pass off: retail keeps `rlwinm r,r,0,29,30` + `cmpwi` unfused where
 *     the pass fuses `rlwinm.` + `bne` (playbook 39).
 *   * the colour byte extraction is the **masked** form, `(color & 0xFF000000) >> 24`; with the
 *     peephole off that emits the raw `clrrwi`+`srwi` (and `rlwinm`+`srwi`+`clrlwi`) chain retail has,
 *     while `(u8)(color >> 24)` is three instructions short.
 *   * `s32 i;` must be declared **before** `u32 color;`: the declaration order decides which of the two
 *     gets r27 and which reuses the dead `enemy`'s r30.
 *   * `s32 mapno = get_now_mapno__Fv();` (not `u8`) is what narrows with `clrlwi` and then compares
 *     `cmpwi`; `(s32)self->type_0x02 == 0x2E` is the same for the `0x2E` test.
 *   * the effect object needs its `+0x34` slot (the sibling `_EFT`'s `dispatch_0x34`) or the work
 *     pointer lands at +0x34 and every later offset shifts.
 *   * the secondary `switch` reads the type into a **local** used as the table index: the type then
 *     lives in a non-r0 register and MWCC emits the `subi`+`cmplwi` range test retail has (with a
 *     direct field read it lands in r0 and becomes two compares).
 *
 * `VEC3`, `MTX34`, `_CP_VECTOR`, `_ENEMY_WORK` and the two effect types are reconstructed minimally
 * (only the offsets this unit reads); the sibling units still carry private copies, so the shared ones
 * belong in `include/` the next time one of them is touched.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80104BD0_fn_80104BD0.c`.
 */

#include "types.h"
#include "nw4r/math.h"

/* ---------------------------------------------------------------------------------------------------
 * types
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` / `MTX34` come from `nw4r/math.h` - one definition, in the owner's header (rule 1).  The
 * header is C-visible, so a `-lang=c` unit can include it. */

/* The rotation source `cpSetRotMatrix` converts into a matrix.  size: 0x0C */
typedef struct _CP_VECTOR {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} _CP_VECTOR;

/* The enemy work object.  Only the bytes this unit reads are named; the full 0x300-byte record is the
 * enemy units' (`D:/WiiExperiment/MH3Disassembly/_ENEMY_WORK.h` gives the size, from the memset at
 * 0x802FFD98).  size: 0x300 */
typedef struct _ENEMY_WORK {
    /* +0x000 */ u8 pad_0x000[0x188];
    /* +0x188 */ VEC3 field_0x188;
    /* +0x194 */ u8 pad_0x194[0x20C - 0x194];
    /* +0x20C */ f32 field_0x20C; /* added to the 5.0 base for one colour group's height */
    /* +0x210 */ f32 field_0x210; /* the other colour group's height base */
    /* +0x214 */ u8 pad_0x214[0x228 - 0x214];
    /* +0x228 */ u16 field_0x228; /* bits 1-2: "damaged" state that shifts some effect types */
} _ENEMY_WORK;

/* The per-family work block the effect object points at (+0x38).  `count` effects are pooled from
 * +0x08; the RGBA at +0x10 and the scale at +0x14 sit past the two the two-effect families use.
 * size: 0x18 */
typedef struct _EM_EFT_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ u32 joint_0x04; /* joint the effects follow; 0xFF means "use the object's own pos" */
    /* +0x08 */ void* effects[2];
    /* +0x10 */ u8 color_r_0x10;
    /* +0x11 */ u8 color_g_0x11;
    /* +0x12 */ u8 color_b_0x12;
    /* +0x13 */ u8 color_a_0x13;
    /* +0x14 */ f32 paramscale_0x14;
} _EM_EFT_WORK;

/* The effect object this unit drives.  size: 0x48 (the sibling `_EFT`; the record continues past what
 * this unit reads) */
typedef struct _EM_EFT {
    /* +0x00 */ u8 pad_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u8 pad_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 pad_0x06[0x18 - 0x06];
    /* +0x18 */ VEC3 pos_0x18;
    /* +0x24 */ _CP_VECTOR rot_0x24;
    /* +0x30 */ _ENEMY_WORK* enemy_0x30;
    /* +0x34 */ void (*dispatch_0x34)(void); /* not read here; the sibling `_EFT` names it */
    /* +0x38 */ _EM_EFT_WORK* work_0x38;
    /* +0x3C */ u8 pad_0x3C[0x44 - 0x3C];
    /* +0x44 */ u8 area_0x44;
} _EM_EFT;

/* ---------------------------------------------------------------------------------------------------
 * externs
 * ------------------------------------------------------------------------------------------------- */

/* C-linkage callees (the map spells these plainly). */
void fn_8005050C(MTX34* mtx);                  /* mtx = identity */
void fn_80043EA8(VEC3* vec);       /* vec = (0, 0, 0) */
void fn_800FBB90(MTX34* mtx, VEC3* pos);
void fn_80041E40(VEC3* dst, const VEC3* src);
void fn_80105314(_EM_EFT* self);
void fn_80105560(_EM_EFT* self);               /* effect creation failed */
void fn_80105564(_EM_EFT* self);

/* C++-mangled callees, declared with the map's spelling. */
extern u8 get_now_mapno__Fv(void);
extern s32 em_work_die_ck__FP11_ENEMY_WORK(_ENEMY_WORK* enemy);
extern void get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3(_ENEMY_WORK* enemy, u32 joint,
                                                                  VEC3* pos);
extern void* res_eft_create__FUsUsUl(u16 id, u16 param, u32 arg);
extern void cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(_CP_VECTOR* rot, MTX34* mtx);
extern void mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(VEC3* vec, MTX34* mtx);
extern u32 get_stg_eft_col__FUcUc(u8 area, u8 which);
extern void SetRootMtx__Q34nw4r2ef6EffectFRCQ34nw4r4math5MTX34(void* effect, const MTX34* mtx);
extern void change_paramscale_eff__FPQ34nw4r2ef6Effectf(void* effect, f32 scale);
extern void change_paramscale_eff_vec3__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(void* effect,
                                                                               VEC3* scale);
extern void setVector3__FPQ34nw4r4math4VEC3fff(VEC3* vec, f32 x, f32 y, f32 z);

/* The type tables and the two scale constants, referenced but owned by another unit's pool (playbook
 * 29: declared, never defined). */
extern u16 lbl_8059E430[];
extern u16 lbl_8059E55C[];
extern f32 lbl_80796778; /* 5.0f */
extern f32 lbl_8079677C; /* 0.7f */
extern f32 lbl_80796780; /* 0.5f */

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

/* Drives one enemy hit-effect object for a frame: spawns the type's effects on the first call, then
 * re-places, re-colours and re-scales them. */
void fn_80104BD0(_EM_EFT* self)
{
    MTX34 mtx;
    VEC3 vec;
    _EM_EFT_WORK* work;
    _ENEMY_WORK* enemy;
    s32 i;
    u32 color;

#pragma peephole off

    fn_8005050C(&mtx);
    fn_80043EA8(&vec);

    work = self->work_0x38;
    enemy = self->enemy_0x30;

    if (enemy != NULL && em_work_die_ck__FP11_ENEMY_WORK(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }

    self->state_0x05++;

    switch (self->type_0x02) {
    case 0x1E:
    case 0x1F:
    case 0x20:
        if (enemy != NULL && (enemy->field_0x228 & 6) != 0) {
            self->type_0x02 += 3;
        }
        break;
    case 0x37:
        if (enemy != NULL && (enemy->field_0x228 & 6) != 0) {
            self->type_0x02 += 1;
        }
        break;
    case 0x39:
    case 0x3A:
    case 0x3B:
    case 0x3C:
        if (enemy != NULL) {
            if ((enemy->field_0x228 & 6) != 0) {
                self->type_0x02 += 0xC;
            } else {
                s32 mapno = get_now_mapno__Fv();
                if (mapno == 4 || mapno == 0xF) {
                    self->type_0x02 += 8;
                }
            }
        }
        break;
    case 0x4E:
    case 0x4F:
    case 0x50:
        if (enemy != NULL && (enemy->field_0x228 & 6) != 0) {
            self->type_0x02 += 0x34;
        }
        break;
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
        if (enemy != NULL && (enemy->field_0x228 & 6) != 0) {
            self->type_0x02 += 4;
        }
        break;
    }

    work->effects[0] = res_eft_create__FUsUsUl(lbl_8059E430[self->type_0x02],
                                               lbl_8059E55C[self->type_0x02], 0);
    if (work->effects[0] == NULL) {
        fn_80105560(self);
        return;
    }

    {
        u8 type = self->type_0x02;

        switch (type) {
        case 0x33:
            work->effects[1] = res_eft_create__FUsUsUl(0xE2, lbl_8059E55C[type], 0);
            if (work->effects[1] == NULL) {
                fn_80105560(self);
                return;
            }
            work->count++;
            break;
        case 0x34:
            work->effects[1] = res_eft_create__FUsUsUl(0x14A, lbl_8059E55C[type], 0);
            if (work->effects[1] == NULL) {
                fn_80105560(self);
                return;
            }
            work->count++;
            break;
        case 0x35:
        case 0x36:
            work->effects[1] = res_eft_create__FUsUsUl(0xE3, lbl_8059E55C[type], 0);
            if (work->effects[1] == NULL) {
                fn_80105560(self);
                return;
            }
            work->count++;
            break;
        }
    }

    cpSetRotMatrix__FP10_CP_VECTORPQ34nw4r4math5MTX34(&self->rot_0x24, &mtx);

    if (work->joint_0x04 != 0xFF) {
        mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(&self->pos_0x18, &mtx);
        fn_800FBB90(&mtx, &self->pos_0x18);
        get_joint_wpos_em__FP11_ENEMY_WORKUlPQ34nw4r4math4VEC3(enemy, work->joint_0x04, &self->pos_0x18);

        switch (self->type_0x02) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x1A:
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x24:
        case 0x25:
        case 0x27:
        case 0x28:
        case 0x2B:
        case 0x31:
        case 0x32:
        case 0x37:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x49:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x7B:
        case 0x7D:
        case 0x7E:
        case 0x7F:
        case 0x80:
        case 0x81:
        case 0x91:
        case 0x94:
            self->pos_0x18.y = lbl_80796778 + enemy->field_0x20C;
            color = get_stg_eft_col__FUcUc(self->area_0x44, 0);
            break;
        case 0x78:
            self->pos_0x18.y = enemy->field_0x210 - lbl_80796778;
            color = get_stg_eft_col__FUcUc(self->area_0x44, 1);
            break;
        case 0x03:
        case 0x26:
        case 0x38:
        case 0x7C:
        default:
            self->pos_0x18.y = lbl_80796778 + enemy->field_0x210;
            color = get_stg_eft_col__FUcUc(self->area_0x44, 1);
            break;
        }
    } else {
        if (enemy != NULL) {
            mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(&self->pos_0x18, &mtx);
            fn_800FBB90(&mtx, &self->pos_0x18);
            fn_80041E40(&self->pos_0x18, &enemy->field_0x188);
        }

        switch (self->type_0x02) {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x1A:
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x24:
        case 0x25:
        case 0x27:
        case 0x28:
        case 0x2B:
        case 0x31:
        case 0x32:
        case 0x37:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x49:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x7B:
        case 0x7D:
        case 0x7E:
        case 0x7F:
        case 0x80:
        case 0x81:
        case 0x91:
        case 0x94:
            color = get_stg_eft_col__FUcUc(self->area_0x44, 0);
            break;
        case 0x26:
        case 0x38:
        default:
            color = get_stg_eft_col__FUcUc(self->area_0x44, 1);
            break;
        }
    }

    fn_80105564(self);

    mtx.m[0][3] += self->pos_0x18.x;
    mtx.m[1][3] += self->pos_0x18.y;
    mtx.m[2][3] += self->pos_0x18.z;

    for (i = 0; i < work->count; i++) {
        SetRootMtx__Q34nw4r2ef6EffectFRCQ34nw4r4math5MTX34(work->effects[i], &mtx);
    }

    self->flag_0x01 = 1;
    work->color_r_0x10 = (color & 0xFF000000) >> 24;
    work->color_g_0x11 = (color & 0x00FF0000) >> 16;
    work->color_b_0x12 = (color & 0x0000FF00) >> 8;
    work->color_a_0x13 = 0xFF;

    if ((s32)self->type_0x02 == 0x2E) {
        setVector3__FPQ34nw4r4math4VEC3fff(&vec, lbl_8079677C, lbl_80796780, lbl_8079677C);
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff_vec3__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(work->effects[i], &vec);
        }
    } else {
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff__FPQ34nw4r2ef6Effectf(work->effects[i], work->paramscale_0x14);
        }
    }

    fn_80105314(self);
}
