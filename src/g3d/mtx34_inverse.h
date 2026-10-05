/* Leaf header for `mtx34_inverse` (0x800883C4), owned by `g3d/g3d_state.cpp`: the 3x4 matrix inverse (rule 2).
 */
#ifndef MHTRI_G3D_MTX34_INVERSE_H
#define MHTRI_G3D_MTX34_INVERSE_H

#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Writes the inverse of the 3x4 matrix `src` into `out`. */
void mtx34_inverse(nw4r::math::MTX34* out, const nw4r::math::MTX34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_MTX34_INVERSE_H */
