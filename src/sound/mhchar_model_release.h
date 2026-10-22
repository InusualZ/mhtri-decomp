/* Leaf header (docs/plan.md 6.5 rule 2): `mhchar_model_release`, which `sound/mhchar` owns (the name is a registry proposal of lane
 * enemy-control; the map row is renamed when it lands).  C linkage. */
#ifndef MHTRI_SOUND_MHCHAR_MODEL_RELEASE_H
#define MHTRI_SOUND_MHCHAR_MODEL_RELEASE_H

#include "types.h"
#include "pl.h" /* MHchar */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E26C4 - releases the MHchar model. */
void mhchar_model_release(MHchar* chr);

#ifdef __cplusplus
}
#endif

#endif
