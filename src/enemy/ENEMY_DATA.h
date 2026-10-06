/* The enemy per-entry data table record `enemy_data_find` (0x803438E4, `ef/eft_slot.cpp`) returns: one home for the
 * type, the union of every reader's view, bytes no reader touches kept as `pad_0xNN`; `enemy/fn_80165FC8.h` includes it.
 *
 * size: 0x40 (lower bound: the highest byte any reader names is +0x39) */
#ifndef MHTRI_ENEMY_ENEMY_DATA_H
#define MHTRI_ENEMY_ENEMY_DATA_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

typedef struct _ENEMY_DATA {
    /* +0x00 */ u8 pad_0x00[0x08];
    /* +0x08 */ u8 field_0x08;   /* `fn_801671AC` latches the work's +0x46C to 0xFF when it is 0xFF;
                                  * `em_act_frame_ck` reads it as the "entry seated" state */
    /* +0x09 */ u8 pad_0x09[0x02];
    /* +0x0B */ u8 field_0x0B;   /* `em_act_frame_ck` asks for the leave transition when it is not 1 */
    /* +0x0C */ u8 pad_0x0C[0x14 - 0x0C];
    /* +0x14 */ u8 field_0x14;   /* `fn_80166DF8` compares it against the work's +0x1E1 */
    /* +0x15 */ u8 pad_0x15[0x02];
    /* +0x17 */ u8 field_0x17;   /* `fn_801671AC` compares it against the work's +0x46C */
    /* +0x18 */ struct _ENEMY_WORK* work_0x18; /* the entry's live enemy work, or NULL */
    /* +0x1C */ u8 pad_0x1C[0x08];
    /* +0x24 */ nw4r::math::VEC3 vec_0x24; /* the entry's seat/aim vector */
    /* +0x30 */ u8 pad_0x30[0x10];
} _ENEMY_DATA;

#endif /* MHTRI_ENEMY_ENEMY_DATA_H */
