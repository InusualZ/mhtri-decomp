/* enemy/em_break_part_data_get.h - the declarations of `em_break_part_data_get`, which `enemy/fn_80138074.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_ENEMY_EM_BREAK_PART_DATA_GET_H
#define MHTRI_ENEMY_EM_BREAK_PART_DATA_GET_H

#include "types.h"

struct _ENEMY_WORK;
struct EmBreakPartCurves;

/* The enemy data's description of one breakable part (`EnemyData::table_0x98`): the lift it flies up by, the part
 * number `em_parts_break` takes and the three model materials it hides.
 * size: 0x20 (a lower bound: only these fields are reached) */
struct EmBreakPartData {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ f32 lift;
    /* +0x0C */ struct EmBreakPartCurves* curves;  /* the spin curves (`em_break_part_spin`) */
    /* +0x10 */ u8 part;
    /* +0x11 */ u8 pad_0x11[0x3];
    /* +0x14 */ u32 mats[3];       /* 0xFFFFFFFF: none */
};


#ifdef __cplusplus
extern "C" {
#endif
struct EmBreakPartData* em_break_part_data_get(struct _ENEMY_WORK* self, u8 index);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_BREAK_PART_DATA_GET_H */
