/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `ef_vec3_normalize_to`, owned by `ef/ef_util.cpp`; spelled as the draw strategies call
 * it.  GUESS names (from the callers' use): `ef_vec3_normalize_to`.
 */
#ifndef MHTRI_EF_EF_VEC3_NORMALIZE_TO_H
#define MHTRI_EF_EF_VEC3_NORMALIZE_TO_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009C484 - normalises `src` into `dst`; NULL for a zero vector. */
VEC3* ef_vec3_normalize_to(VEC3* dst, const VEC3* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_VEC3_NORMALIZE_TO_H */
