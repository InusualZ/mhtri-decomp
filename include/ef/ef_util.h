/*
 * Declarations for the symbols `src/ef/ef_util.cpp` owns (docs/plan.md 6.5 rule 2).  `fn_8009CD64` is the matrix-axis scale helper at the
 * range's tail (0x8009CD64..0x8009CDBC, the 88 bytes phase 4 gave the unit); `ef/ef_emitter.cpp` calls it.
 */
#ifndef MHTRI_EF_EF_UTIL_H
#define MHTRI_EF_EF_UTIL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 fn_8009CD64(const f32* mtx, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_UTIL_H */
