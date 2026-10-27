/* Leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s per-slot level update (registry name; GUESS). */
#ifndef MHTRI_SOUND_SE_SLOT_LEVEL_CALC_H
#define MHTRI_SOUND_SE_SLOT_LEVEL_CALC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _se_w;
struct SeSlot;
/* 0x800D92E4 - recomputes the bank, level and fade volumes of `slot` from the listener distance. */
void se_slot_level_calc(struct _se_w* work, struct SeSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_SLOT_LEVEL_CALC_H */
