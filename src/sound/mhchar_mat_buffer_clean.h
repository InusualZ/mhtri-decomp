/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s mhchar_mat_buffer_clean */
#ifndef MHTRI_SOUND_MHCHAR_MAT_BUFFER_CLEAN_H
#define MHTRI_SOUND_MHCHAR_MAT_BUFFER_CLEAN_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E30DC - drops material `index`'s copied blocks selected by `option`. */
void mhchar_mat_buffer_clean(MHchar* chr, u32 index, u32 option);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MAT_BUFFER_CLEAN_H */
