/* The player hit-sphere unit `Pl/pl_hit_sphere.cpp`: the segment and capsule box builders, their records, and the
 * point-in-sphere test.  The box builders' map names are plain, so they take C linkage; `hit_point_sphr` is mangled.
 */
#ifndef MHTRI_PL_PL_HIT_SPHERE_H
#define MHTRI_PL_PL_HIT_SPHERE_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* A segment: the two endpoints `pl_segment_box_set` copies.  size: 0x18 (two 0xC-byte `copyVec3` records) */
struct PlSegment {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
};

/* The box `pl_segment_box_set` fills: the endpoints, `end - start`, the midpoint (the sum scaled by the 0.5 at
 * .sdata2 0x8079A310) and the distance from `start` to it.  size: 0x34 (the last store is the f32 at +0x30) */
struct PlSegmentBox {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
    /* +0x18 */ nw4r::math::VEC3 dir;
    /* +0x24 */ nw4r::math::VEC3 center;
    /* +0x30 */ f32 half_length;
};

/* A capsule: two endpoints and the radius `pl_capsule_box_set` copies.  size: 0x1C (the f32 read at +0x18) */
struct PlCapsule {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
    /* +0x18 */ f32 radius;
};

/* The box `pl_capsule_box_set` fills: the capsule, `end - start`, the midpoint and the bounding reach (the
 * distance from `start` to the midpoint plus the radius).  size: 0x38 (the last store is the f32 at +0x34) */
struct PlCapsuleBox {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
    /* +0x18 */ f32 radius;
    /* +0x1C */ nw4r::math::VEC3 dir;
    /* +0x28 */ nw4r::math::VEC3 center;
    /* +0x34 */ f32 reach;
};

extern "C" {

/* 0x8028F4B4 (0xA4): fills `box` from the segment's endpoints. */
void pl_segment_box_set(const PlSegment* seg, PlSegmentBox* box);

/* 0x8028F558 (0xC4): fills `box` from the capsule. */
void pl_capsule_box_set(const PlCapsule* capsule, PlCapsuleBox* box);

}

/* 0x8028F61C (0x50): 1 when `point` lies inside the sphere of `radius` around `center`. */
u32 hit_point_sphr(nw4r::math::VEC3* point, nw4r::math::VEC3* center, f32 radius);

#endif /* __cplusplus */

#endif /* MHTRI_PL_PL_HIT_SPHERE_H */
