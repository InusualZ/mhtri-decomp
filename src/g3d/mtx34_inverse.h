/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `mtx34_inverse` with its result, owned by `g3d/g3d_state.cpp`; spelled as the draw strategies call
 * it (`g3d/g3d_state.h` drops the
 * result the stripe draw tests).  GUESS names (from the callers' use): none (the map name).
 */
#ifndef MHTRI_G3D_MTX34_INVERSE_H
#define MHTRI_G3D_MTX34_INVERSE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800883C4 - inverts `src` into `out`; 0 when the matrix is singular. */
u32 mtx34_inverse(MTX34* out, const MTX34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_MTX34_INVERSE_H */
