/* ef/effect_types.h - the types and macros `ef/effect.cpp` and `ef/eft_model_slot.cpp` share (one definition each). */
#ifndef MHTRI_EF_EFFECT_TYPES_H
#define MHTRI_EF_EFFECT_TYPES_H

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/scnmdl.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "g3d/fn_80063888.h" /* fn_80064820, owned by g3d/fn_80063888.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */

/* The effect state `fn_800F9D80`/`eft_state_flags_set` read and write. */
typedef struct EftFrameState {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 flags_0x04;
    /* +0x05 */ u8 mode_0x05;
    /* +0x06 */ u8 pad_0x06[0x16];
    /* +0x1C */ f32 value_0x1C;
} EftFrameState; /* size: 0x20 */
#endif /* MHTRI_EF_EFFECT_TYPES_H */
