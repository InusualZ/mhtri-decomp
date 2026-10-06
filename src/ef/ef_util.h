/* ef/ef_util.h - the symbols `ef/ef_util.cpp` owns: the matrix-axis scale helper `fn_8009CD64` at the range's
 * tail (0x8009CD64-0x8009CDBC), which `ef/ef_emitter.cpp` calls. */
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
