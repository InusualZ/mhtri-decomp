/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s mhchar_mat_color_set_all */
#ifndef MHTRI_SOUND_MHCHAR_MAT_COLOR_SET_ALL_H
#define MHTRI_SOUND_MHCHAR_MAT_COLOR_SET_ALL_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E29A8 - sets every material's colour channel `channel` to `*color`. */
void mhchar_mat_color_set_all(MHchar* chr, _GXChannelID channel, _GXColor* color, bool keep);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MAT_COLOR_SET_ALL_H */
