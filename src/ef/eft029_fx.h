/* ef/eft029_fx.h - the declarations `ef/eft029_fx.cpp` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT029_FX_H
#define MHTRI_EF_EFT029_FX_H

#include "types.h"

#ifdef __cplusplus

struct _PLW;

/* 0x8011AD84 (0x28): passes the player, the eft029 type and the scale on to the family's scale worker; C++ scope, so
 * this is the callable spelling of the map name `eft029_set_scale__FP4_PLWUcf` (rule 9). */
void eft029_set_scale(struct _PLW* plw, u8 type, f32 scale);

#endif

#endif /* MHTRI_EF_EFT029_FX_H */
