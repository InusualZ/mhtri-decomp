/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `vec3_assign`, owned by `mh3_pad.cpp`; spelled as the draw strategies call
 * it.  GUESS names (from the callers' use): `vec3_assign`.
 */
#ifndef MHTRI_MH3_PAD_VEC3_ASSIGN_H
#define MHTRI_MH3_PAD_VEC3_ASSIGN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80041E70 - copies the three components of `src` into `dst`. */
void vec3_assign(VEC3* dst, const VEC3* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MH3_PAD_VEC3_ASSIGN_H */
