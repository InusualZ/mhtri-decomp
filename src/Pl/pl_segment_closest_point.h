/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_coll.cpp`'s segment closest-point helper (C linkage, the map row is unmangled). */
#ifndef MHTRI_PL_PL_SEGMENT_CLOSEST_POINT_H
#define MHTRI_PL_PL_SEGMENT_CLOSEST_POINT_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028F938 - writes the point of the segment `start`..`end` closest to `point` into `out`. */
f32 pl_segment_closest_point(VEC3* start, VEC3* end, VEC3* point, VEC3* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SEGMENT_CLOSEST_POINT_H */
