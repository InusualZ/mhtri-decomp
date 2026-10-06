/* OS/PSMTXRotAxisRad.h - `PSMTXRotAxisRad`, which `OS/FindContainHeap_.c` owns (docs/plan.md 6.5 rule 2, leaf
 *   header). */
#ifndef MHTRI_OS_PSMTXROTAXISRAD_H
#define MHTRI_OS_PSMTXROTAXISRAD_H

#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C6290 - builds a rotation of `rad` radians about `axis`. */
void PSMTXRotAxisRad(MTX34* m, const VEC3* axis, f32 rad);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_PSMTXROTAXISRAD_H */
