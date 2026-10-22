/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8`'s `sqrtf_thunk`, which `Pl/pl_coll.cpp` calls.
 * C linkage (the map row is a plain name).  GUESS name: the body is a tail branch to `sqrtf`. */
#ifndef MHTRI_SQRTF_THUNK_H
#define MHTRI_SQRTF_THUNK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80050714 - `sqrtf(x)` through a tail branch. */
f32 sqrtf_thunk(f32 x);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SQRTF_THUNK_H */
