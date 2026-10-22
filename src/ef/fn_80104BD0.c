/* ef/fn_80104BD0.c - the per-frame handler of the enemy hit-effect controller: it spawns the one or two `nw4r::ef`
 *   effects the type selects, drives their root matrices from the enemy joint (or the enemy's position), colours
 *   them from the stage effect-colour table and scales them per frame.
 * RANGE. .text 0x80104BD0-0x80105314 (1 function); extab 0x8000BFB4-0x8000BFBC, extabindex 0x80025E78-0x80025E84,
 *   .data 0x8059E430-0x8059E688, .sdata2 0x80796778-0x80796784.
 * FLAGS. `cflags_main`; `#pragma peephole off` from the first statement on (retail keeps `rlwinm` + `cmpwi` unfused,
 *   playbook 39).
 * NAMES. The map has only the `fn_` stem.  The file is C; mangled callees are declared by their map spelling.
 *   GUESS: `em_hit_eft_effect_ids`/`em_hit_eft_param_ids` (map `lbl_8059E430`/`lbl_8059E55C`, the
 *   `res_eft_create` arguments indexed by the type).
 * RESIDUALS. none: the row and the claimed sections match (`.data` the two tables, `.sdata2` the body's literals).
 * SHAPES. The two type switches enumerate every value of each non-default run (MWCC merges consecutive same-body
 *   cases into a run, so a missing value falls to `default` and changes the comparison tree).
 *   The colour bytes are extracted masked (`(color & 0xFF000000) >> 24`): `(u8)(color >> 24)` is three instructions
 *   short.
 *   `s32 i;` is declared before `u32 color;` (the order decides which reuses the dead `enemy` register).
 *   `s32 mapno = get_now_mapno__Fv();` (not `u8`) narrows with `clrlwi` and compares `cmpwi`, as
 *   `(s32)self->type_0x02 == 0x2E` does.
 *   The effect object keeps its `+0x34` slot (`dispatch_0x34`) so the work pointer stays at +0x38.
 *   The secondary switch reads the type into a local used as the table index (MWCC then emits retail's
 *   `subi` + `cmplwi` range test).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/cp_vector.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */

/* ---------------------------------------------------------------------------------------------------
 * types
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` / `MTX34` come from `nw4r/math.h` - one definition, in the owner's header (rule 1).  The
 * header is C-visible, so a `-lang=c` unit can include it. */

/* `_CP_VECTOR`, the rotation source `cpSetRotMatrix` converts into a matrix, comes from `ef/cp_vector.h` - one
 * definition, in the owner's header (rule 1). */

/* The enemy work object.  Only the bytes this unit reads are named; the full 0x300-byte record is the
 * enemy units' (`<disassembly>/_ENEMY_WORK.h` gives the size, from the memset at
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
                  /* mtx = identity */
       /* vec = (0, 0, 0) */
void mtx34_set_trans(MTX34* mtx, VEC3* pos);
void eft_em_effects_move(_EM_EFT* self);
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

/* The unit's `.data`: the `res_eft_create` effect id and parameter id per hit-effect type (`self->type_0x02`,
 * 150 types). */
u16 em_hit_eft_effect_ids[150] = {
    0x0016, 0x0017, 0x00F8, 0x011F, 0x0129, 0x012F, 0x012E, 0x0130, 0x0132, 0x0149, 0x011F, 0x0125, 0x014D, 0x014E, 0x014F,
    0x0150, 0x0151, 0x0152, 0x0153, 0x0154, 0x0155, 0x0156, 0x00E1, 0x00E2, 0x0141, 0x0142, 0x01ED, 0x01E3, 0x01EB, 0x01EC,
    0x01EE, 0x01EF, 0x01F0, 0x01F1, 0x01F2, 0x01F3, 0x0121, 0x0120, 0x014A, 0x02B1, 0x0346, 0x0361, 0x0362, 0x02B3, 0x0151,
    0x0151, 0x01A3, 0x0416, 0x00E3, 0x04B9, 0x04CA, 0x00E1, 0x00E2, 0x0142, 0x0141, 0x04F1, 0x04F2, 0x04F3, 0x04F4, 0x04F5,
    0x04F6, 0x04F7, 0x04F8, 0x04F9, 0x04FA, 0x04FB, 0x04FC, 0x04FD, 0x04FE, 0x04FF, 0x0500, 0x0501, 0x0502, 0x0503, 0x0503,
    0x0504, 0x052C, 0x055B, 0x055D, 0x055E, 0x055F, 0x05DA, 0x05DE, 0x05E1, 0x05E2, 0x05E5, 0x05E6, 0x05E7, 0x0602, 0x0603,
    0x0604, 0x0605, 0x0608, 0x060B, 0x060C, 0x0609, 0x060A, 0x0617, 0x0618, 0x0622, 0x0623, 0x0624, 0x05E8, 0x05E9, 0x05EA,
    0x05EB, 0x063F, 0x0699, 0x069B, 0x069C, 0x069D, 0x06A0, 0x06C9, 0x06CA, 0x06CB, 0x06CC, 0x06CD, 0x06CE, 0x06CF, 0x06D0,
    0x03ED, 0x0152, 0x06E3, 0x06E4, 0x06E5, 0x0734, 0x073C, 0x073D, 0x073F, 0x0790, 0x0990, 0x0991, 0x0992, 0x09E5, 0x09E6,
    0x09E7, 0x09E8, 0x09E9, 0x09EA, 0x09EB, 0x09EC, 0x09ED, 0x09EE, 0x09EF, 0x09F0, 0x0A16, 0x000F, 0x0A4A, 0x0A4C, 0x0000,
};

u16 em_hit_eft_param_ids[150] = {
    0x001F, 0x0004, 0x001F, 0x0000, 0x001F, 0x001F, 0x001F, 0x001F, 0x001F, 0x0000, 0x0000, 0x0000, 0x0028, 0x0028, 0x0028,
    0x0028, 0x0028, 0x0028, 0x0028, 0x0028, 0x0028, 0x0028, 0x001A, 0x001A, 0x001A, 0x001A, 0x0033, 0x0033, 0x0033, 0x0033,
    0x0033, 0x0033, 0x0033, 0x0033, 0x0033, 0x0033, 0x0000, 0x0000, 0x001A, 0x0042, 0x0042, 0x0032, 0x0032, 0x0042, 0x0028,
    0x0028, 0x0028, 0x0022, 0x001A, 0x005E, 0x005B, 0x001A, 0x001A, 0x001A, 0x001A, 0x0042, 0x0042, 0x005E, 0x005E, 0x005E,
    0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005E,
    0x005E, 0x0062, 0x005B, 0x005B, 0x005B, 0x005B, 0x0042, 0x0042, 0x0042, 0x0042, 0x005A, 0x005A, 0x005A, 0x0071, 0x0071,
    0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x005A, 0x005A, 0x005A,
    0x005A, 0x0042, 0x0071, 0x0071, 0x0071, 0x0071, 0x0071, 0x005B, 0x005B, 0x005B, 0x005B, 0x005B, 0x005B, 0x005B, 0x005B,
    0x0024, 0x0028, 0x005B, 0x005B, 0x005B, 0x005B, 0x0071, 0x0071, 0x0071, 0x005B, 0x005B, 0x005B, 0x005B, 0x00BD, 0x00BD,
    0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x00BD, 0x0071, 0x0000, 0x00BD, 0x0042, 0x0000,
};

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

    MTX34_ctor(&mtx);
    VEC3_ctor(&vec);

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

    work->effects[0] = res_eft_create__FUsUsUl(em_hit_eft_effect_ids[self->type_0x02],
                                               em_hit_eft_param_ids[self->type_0x02], 0);
    if (work->effects[0] == NULL) {
        fn_80105560(self);
        return;
    }

    {
        u8 type = self->type_0x02;

        switch (type) {
        case 0x33:
            work->effects[1] = res_eft_create__FUsUsUl(0xE2, em_hit_eft_param_ids[type], 0);
            if (work->effects[1] == NULL) {
                fn_80105560(self);
                return;
            }
            work->count++;
            break;
        case 0x34:
            work->effects[1] = res_eft_create__FUsUsUl(0x14A, em_hit_eft_param_ids[type], 0);
            if (work->effects[1] == NULL) {
                fn_80105560(self);
                return;
            }
            work->count++;
            break;
        case 0x35:
        case 0x36:
            work->effects[1] = res_eft_create__FUsUsUl(0xE3, em_hit_eft_param_ids[type], 0);
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
        mtx34_set_trans(&mtx, &self->pos_0x18);
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
            self->pos_0x18.y = 5.0f + enemy->field_0x20C;
            color = get_stg_eft_col__FUcUc(self->area_0x44, 0);
            break;
        case 0x78:
            self->pos_0x18.y = enemy->field_0x210 - 5.0f;
            color = get_stg_eft_col__FUcUc(self->area_0x44, 1);
            break;
        case 0x03:
        case 0x26:
        case 0x38:
        case 0x7C:
        default:
            self->pos_0x18.y = 5.0f + enemy->field_0x210;
            color = get_stg_eft_col__FUcUc(self->area_0x44, 1);
            break;
        }
    } else {
        if (enemy != NULL) {
            mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(&self->pos_0x18, &mtx);
            mtx34_set_trans(&mtx, &self->pos_0x18);
            copyVec3(&self->pos_0x18, &enemy->field_0x188);
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
        setVector3__FPQ34nw4r4math4VEC3fff(&vec, 0.7f, 0.5f, 0.7f);
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff_vec3__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(work->effects[i], &vec);
        }
    } else {
        for (i = 0; i < work->count; i++) {
            change_paramscale_eff__FPQ34nw4r2ef6Effectf(work->effects[i], work->paramscale_0x14);
        }
    }

    eft_em_effects_move(self);
}
