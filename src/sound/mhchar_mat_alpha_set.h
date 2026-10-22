/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s mhchar_mat_alpha_set */
#ifndef MHTRI_SOUND_MHCHAR_MAT_ALPHA_SET_H
#define MHTRI_SOUND_MHCHAR_MAT_ALPHA_SET_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E2A20 - sets every material's colour-channel `channel` alpha to `alpha` and its blend mode (hidden at 0xFF) (GUESS name). */
void mhchar_mat_alpha_set(MHchar* model, u32 channel, u8 alpha, u32 keep);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MAT_ALPHA_SET_H */
