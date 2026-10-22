/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s `mhchar_model_mtx_update`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_SOUND_MHCHAR_MODEL_MTX_UPDATE_H
#define MHTRI_SOUND_MHCHAR_MODEL_MTX_UPDATE_H

#include "types.h"

struct MHchar;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E0914 - rebuilds the character model's world matrix from its position and rotation (cpSetRotMatrixZXY, ScnObj::SetMtx) */
void mhchar_model_mtx_update(struct MHchar* chr);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MODEL_MTX_UPDATE_H */
