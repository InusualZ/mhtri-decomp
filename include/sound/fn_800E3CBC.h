/*
 * sound/fn_800E3CBC.h - the declarations `src/sound/fn_800E3CBC.cpp` owns (docs/plan.md 6.5 rule 2).
 * `prim_init_all` (0x800E3D40, the map's `prim_init_all__Fv`) resets the primitive draw state every
 * frame; added with `quest/arenatask.cpp`, the first consumer.
 */
#ifndef MHTRI_SOUND_FN_800E3CBC_H
#define MHTRI_SOUND_FN_800E3CBC_H

#include "types.h"

#ifdef __cplusplus
void prim_init_all(void);
#endif

#endif /* MHTRI_SOUND_FN_800E3CBC_H */
