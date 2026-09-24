#ifndef MHTRI_EF_EF_PARTICLEMANAGER_H
#define MHTRI_EF_EF_PARTICLEMANAGER_H

#include "types.h"
#include "nw4r/math.h"

/* Declarations for the symbols `src/ef/ef_particlemanager.cpp` owns (docs/plan.md 6.5, rule 2).  The
 * signature is the one the consumer (ef/ef_drawfreestrategy.cpp) calls with; the owner's stub definition
 * must match it.  C-visible, kept minimal. */
#ifdef __cplusplus
extern "C" {
#endif

void fn_800AE360(void* target, MTX34* out); /* the per-particle transform fn_800BE3C0 reads */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLEMANAGER_H */
