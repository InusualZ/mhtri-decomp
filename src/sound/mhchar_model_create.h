/* Leaf header (docs/plan.md 6.5 rule 2): `mhchar_model_create`, which `sound/mhchar` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_SOUND_MHCHAR_MODEL_CREATE_H
#define MHTRI_SOUND_MHCHAR_MODEL_CREATE_H

#include "types.h"
#include "pl.h" /* MHchar */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E2228 - builds the MHchar model from a res file, flags and light slot. */
void mhchar_model_create(MHchar* chr, u32* file, u32 flags, u8 light);

#ifdef __cplusplus
}
#endif

#endif
