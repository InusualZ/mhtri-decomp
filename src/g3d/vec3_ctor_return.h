/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_scnroot.cpp`'s `vec3_ctor_return`, which `Pl/pl_coll.cpp` calls.
 * C linkage (the owner defines it `extern "C"`).  GUESS name (the owner's unit header). */
#ifndef MHTRI_G3D_VEC3_CTOR_RETURN_H
#define MHTRI_G3D_VEC3_CTOR_RETURN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800832F8 - constructs the vector at `pVec` and returns it. */
nw4r::math::VEC3* vec3_ctor_return(nw4r::math::VEC3* pVec);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_VEC3_CTOR_RETURN_H */
