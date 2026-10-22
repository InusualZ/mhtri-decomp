/* Leaf header (docs/plan.md 6.5 rule 2): `mhchar_draw_mode_set`, which `sound/mhchar` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_SOUND_MHCHAR_DRAW_MODE_SET_H
#define MHTRI_SOUND_MHCHAR_DRAW_MODE_SET_H

#include "types.h"
#include "pl.h" /* MHchar */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E0BE8 - stores the mode at +0x38. */
void mhchar_draw_mode_set(MHchar* chr, s32 mode);

#ifdef __cplusplus
}
#endif

#endif
