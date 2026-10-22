/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll.cpp`'s pl_area_ground_probe */
#ifndef MHTRI_PL_PL_AREA_GROUND_PROBE_H
#define MHTRI_PL_PL_AREA_GROUND_PROBE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80291C50 - probes the ground under `pos` in area `area` into `out`; nonzero on a hit (GUESS name). */
struct EmPopGround;
u8 pl_area_ground_probe(nw4r::math::VEC3* pos, u8 area, struct EmPopGround* out, s32 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_AREA_GROUND_PROBE_H */
