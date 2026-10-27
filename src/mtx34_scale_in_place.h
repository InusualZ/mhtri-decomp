/* mtx34_scale_in_place.h - the declaration of `mtx34_scale_in_place`, which the root unit at 0x8004CAD8 owns
 * (docs/plan.md 6.5 rule 2), for a consumer that cannot include that unit's whole header. */
#ifndef MHTRI_MTX34_SCALE_IN_PLACE_H
#define MHTRI_MTX34_SCALE_IN_PLACE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80050708 - scales `mtx` in place by `scale`. */
nw4r::math::MTX34* mtx34_scale_in_place(nw4r::math::MTX34* mtx, const nw4r::math::VEC3* scale);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MTX34_SCALE_IN_PLACE_H */
