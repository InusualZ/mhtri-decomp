/* OS/PSMTXCopy.h - the 3x4 matrix copy and concatenation entry points `OS/FindContainHeap_.c` owns (docs/plan.md
 *   6.5 rule 2, leaf header).  PSMTXConcatArray is a GUESS (0x804C5D50: the SDK's MTXConcatArray, `a * src[i]` into
 *   `dst[i]` for a count). */
#ifndef MHTRI_OS_PSMTXCOPY_H
#define MHTRI_OS_PSMTXCOPY_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C5C40 - copies the matrix `src` to `dst`. */
void PSMTXCopy(const MTX34* src, MTX34* dst);

/* 0x804C5C80 - `ab = a * b`. */
void PSMTXConcat(const MTX34* a, const MTX34* b, MTX34* ab);

/* 0x804C5D50 - `dst[i] = a * src[i]` for `count` matrices. */
void PSMTXConcatArray(const MTX34* a, const MTX34* src, MTX34* dst, u32 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_PSMTXCOPY_H */
