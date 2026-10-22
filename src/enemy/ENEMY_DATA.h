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
    /* +0x0C */ u8 pad_0x0C[0x0F - 0x0C];
    /* +0x0F */ u8 field_0x0F;   /* `enemy/em022_prog.cpp`: 3 picks the +0x10 area over the +0x14 one */
    /* +0x10 */ u8 field_0x10;   /* the alternate area `enemy/em022_prog.cpp` moves to (0xFF = none) */
    /* +0x11 */ u8 pad_0x11[0x13 - 0x11];
    /* +0x13 */ u8 map_0x13;     /* the map `enemy/em010_prog.cpp`'s mini step hands `em_area_entry_enter_ck` */
    /* +0x14 */ u8 field_0x14;   /* `fn_80166DF8` compares it against the work's +0x1E1 */
    /* +0x15 */ u8 field_0x15;   /* the second area byte `enemy/em022_prog.cpp` copies with +0x14 */
    /* +0x16 */ u8 highlight_0x16;   /* set while an enemy-control event highlights the entry (`emc_highlight_set`) */
    /* +0x17 */ u8 field_0x17;   /* `fn_801671AC` compares it against the work's +0x46C */
    /* +0x18 */ struct _ENEMY_WORK* work_0x18; /* the entry's live enemy work, or NULL */
    /* +0x1C */ u8 pad_0x1C[0x06];
    /* +0x22 */ s16 field_0x22;  /* `enemy/em027_prog.cpp`'s frame update takes the hold when it is 1 */
    /* +0x24 */ nw4r::math::VEC3 vec_0x24; /* the entry's seat/aim vector */
    /* +0x30 */ f32 speed_0x30;  /* the target weave reach `em_target_pos_set` latches (`enemy/em_common.cpp`) */
    /* +0x34 */ u8 pad_0x34[0x36 - 0x34];
    /* +0x36 */ u8 field_0x36;   /* the model block index (0xFF: none) `em_target_pos_set` hands `eft_def_model_block`;
                                 * the value `enemy/em022_prog.cpp`'s state query kind 5 reports */
    /* +0x37 */ u8 pad_0x37[0x39 - 0x37];
    /* +0x39 */ u8 field_0x39;   /* the sighting limit `enemy/em022_prog.cpp` compares the work's +0x46E against */
    /* +0x3A */ u8 pad_0x3A[0x40 - 0x3A];
} _ENEMY_DATA;

#endif /* MHTRI_ENEMY_ENEMY_DATA_H */
