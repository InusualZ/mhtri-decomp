/* Pl/pl_segment_sphere_hit.h - the declarations of `pl_segment_sphere_hit` and `pl_box_point_dist_sq`, which
 * `Pl/pl_hit_sphere.cpp` owns (docs/plan.md 6.5 rule 2), with the argument views `enemy/em_common.cpp` calls them
 * through; `pl_box_point_dist_sq` is declared as the owner defines it.  `pl_ground_probe` (`Pl/pl_coll.cpp`'s) comes
 * from its own leaf header so this one stays one unit's. */
#ifndef MHTRI_PL_PL_SEGMENT_SPHERE_HIT_H
#define MHTRI_PL_PL_SEGMENT_SPHERE_HIT_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/pl_ground_probe.h"

struct PlBox;
struct PlSegmentBox;
struct EmSightSphere;

#ifdef __cplusplus
extern "C" {
#endif
u8 pl_segment_sphere_hit(struct PlSegmentBox* box, struct EmSightSphere* sphere, s32 a, s32 b);
f32 pl_box_point_dist_sq(struct PlBox* box, const nw4r::math::VEC3* point, f32* param);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_SEGMENT_SPHERE_HIT_H */
