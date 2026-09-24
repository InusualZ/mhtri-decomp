/*
 * Declarations owned by `ef/eft001.cpp` (docs/plan.md 6.5 rule 2): the symbols the eft001 cluster
 * defines that its ef siblings call.  A consumer includes this header instead of declaring the symbol
 * itself.  Keep it minimal.
 */
#ifndef MHTRI_EF_EFT001_H
#define MHTRI_EF_EFT001_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Writes a VEC3 into an MTX34's translation column (`m[0][3]`, `m[1][3]`, `m[2][3]`); the family's
 * spawn handlers call it after they build a rotation matrix for a placed model. */
void fn_800FBB90(nw4r::math::MTX34* m, nw4r::math::VEC3* v);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT001_H */
