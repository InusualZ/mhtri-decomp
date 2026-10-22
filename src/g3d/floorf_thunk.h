/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_resanmlight.cpp`'s `floorf_thunk`, which `Pl/pl_coll.cpp` calls.
 * C linkage (the map row is a plain name).  GUESS name: the body is a tail branch to `floorf`. */
#ifndef MHTRI_G3D_FLOORF_THUNK_H
#define MHTRI_G3D_FLOORF_THUNK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8008FFFC - `floorf(x)` through a tail branch. */
f32 floorf_thunk(f32 x);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FLOORF_THUNK_H */
