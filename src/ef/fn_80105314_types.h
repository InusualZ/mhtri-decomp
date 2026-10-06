/* ef/fn_80105314_types.h - the types and macros `ef/fn_80105314.cpp` and `ef/eft013_fx.cpp` share. */
#ifndef MHTRI_EF_FN_80105314_TYPES_H
#define MHTRI_EF_FN_80105314_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"
#include "sound/fn_800D7F54.h"
#include "ef/effect.h"
#include "ef/eft004.h"
#include "ef/fn_80105314.h"
#include "ef/eft_res.h"
#include "ef/fn_8010BDE4.h"
#include "gx.h"
#include "pl.h"
#include "unsplit/enemy.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */

/* ---------------------------------------------------------------------------------------------------
 * the 72-byte effect object and its pool blocks (the `_EFT` shape of ef/eft002.cpp, named per-unit so
 * it is not a second definition of that type - docs/plan.md 6.5 rule 1)
 * ------------------------------------------------------------------------------------------------- */

/* The effect object the `eft002` / `fn_800FCED4` / `fn_80104BD0` families all drive.  Only the offsets
 * this unit reads are named. size: 0x48 */
struct _EFT013 {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ _CP_VECTOR rot_0x24;
    /* +0x30 */ _ENEMY_WORK* source_0x30;
    /* +0x34 */ void (*dispatch_0x34)(_EFT013*);
    /* +0x38 */ void* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(_EFT013*);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* size: 0x48 */

/* Pool block of the `fn_80105970` / `fn_801068E4` family: a count, the effect array at +0x04, the
 * parameter scale at +0x08 and the packed colour at +0x0C. */
struct _EFT013_POOL {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
    /* +0x08 */ f32 scale_0x08;
    /* +0x0C */ _GXColor color_0x0C;
};
#endif /* MHTRI_EF_FN_80105314_TYPES_H */
