/* OS/PSMTXInverse.h - `PSMTXInverse`, which `OS/FindContainHeap_.c` owns (docs/plan.md 6.5 rule 2, leaf header;
 *   caller: g3d/g3d_state.cpp). */
#ifndef MHTRI_OS_PSMTXINVERSE_H
#define MHTRI_OS_PSMTXINVERSE_H

#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C5EE0 - inverts `src` into `inv`; 0 when `src` is singular. */
u32 PSMTXInverse(const MTX34* src, MTX34* inv);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_PSMTXINVERSE_H */
