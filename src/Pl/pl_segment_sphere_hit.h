/* Pl/pl_segment_sphere_hit.h - the declarations of `pl_segment_sphere_hit`, `pl_ground_probe`, `pl_box_point_dist_sq`, which `Pl/pl_coll.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_PL_PL_SEGMENT_SPHERE_HIT_H
#define MHTRI_PL_PL_SEGMENT_SPHERE_HIT_H

#include "types.h"
#include "nw4r/math.h"

struct PlBox;
struct PlSegmentBox;
struct EmSightSphere;

#ifdef __cplusplus
extern "C" {
#endif
u8 pl_segment_sphere_hit(struct PlSegmentBox* box, struct EmSightSphere* sphere, s32 a, s32 b);
s32 pl_ground_probe(nw4r::math::VEC3* pos, u16* water, f32* floor, f32* surface, f32* surface2, u8* ceiling, u32 mask);
f32 pl_box_point_dist_sq(struct PlBox* box, nw4r::math::VEC3* point, nw4r::math::VEC3* near);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SEGMENT_SPHERE_HIT_H */
