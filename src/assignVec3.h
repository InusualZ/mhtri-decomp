/* Leaf header (docs/plan.md 6.5 rule 2): `fn_8004CAD8`'s `assignVec3`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_ASSIGNVEC3_H
#define MHTRI_ASSIGNVEC3_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80051490 (fn_8004CAD8): copies `src` into `dst`. */
void assignVec3(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ASSIGNVEC3_H */
