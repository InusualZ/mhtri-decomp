/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s mhchar_mat_num_get */
#ifndef MHTRI_SOUND_MHCHAR_MAT_NUM_GET_H
#define MHTRI_SOUND_MHCHAR_MAT_NUM_GET_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E28E4 - the model's material count (`ResMdl::GetResMatNumEntries` on its +0x114 model) (GUESS name). */
u32 mhchar_mat_num_get(MHchar* model);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MAT_NUM_GET_H */
