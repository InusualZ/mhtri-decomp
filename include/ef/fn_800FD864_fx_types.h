/* Types and macros the units cut from `ef/eft004.cpp` share (hoisted at phase 4 so each is defined once). */
#ifndef MHTRI_EF_FN_800FD864_FX_TYPES_H
#define MHTRI_EF_FN_800FD864_FX_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/fn_80101DF4.h"
#include "ef/eft007.h"
#include "sound/fn_800D7F54.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the effect record and the player work the handlers take
 * ------------------------------------------------------------------------------------------------- */

/* The 0x48-byte game effect record. `work_0x38` is the family-specific pool block; each function casts
 * it to the pool layout it owns. size: 0x48 */
struct Eft004 {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;
    /* +0x02 */ u8 type_0x02;
    /* +0x03 */ u8 phase_0x03;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 state_0x05;
    /* +0x06 */ u8 unused_0x06;
    /* +0x07 */ u8 flag_0x07;
    /* +0x08 */ u8 byte_0x08;
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;
    /* +0x10 */ s32 field_0x10;
    /* +0x14 */ u8 unused_0x14[0x18 - 0x14];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ void* owner_0x30;
    /* +0x34 */ void (*dispatch_0x34)(Eft004*);
    /* +0x38 */ void* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(Eft004*);
    /* +0x44 */ u8 area_0x44;
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* The pool block the `push_eft_effect_heap_num` release helpers walk: a count followed by the effect
 * handles. size: 0x08 - lower bound, an approximation (the pool continues past what this unit reads) */
struct EftEffectPool {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
};
#endif /* MHTRI_EF_FN_800FD864_FX_TYPES_H */
