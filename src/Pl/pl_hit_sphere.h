/* The player shape-test unit `Pl/pl_hit_sphere.cpp`: the segment and capsule box builders, their records, the
 * point-in-sphere test, and the sphere/capsule/segment/plane/triangle contact and push tests.  Every map name but
 * `hit_point_sphr` is plain, so the functions take C linkage.
 */
#ifndef MHTRI_PL_PL_HIT_SPHERE_H
#define MHTRI_PL_PL_HIT_SPHERE_H

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/math_geometry.h"   /* nw4r::math::PLANE */

/* One 0x10-byte placed sphere: an `nw4r::math::VEC3` position plus the radius the overlap tests read at +0xC.  The
 * body-shape lists (`Pl/pl_coll.cpp`'s `pl_coll_body_shape`) are runs of these ended by a radius of -1.  size: 0x10 */
struct HitSphere {
    /* +0x0 */ nw4r::math::VEC3 pos;
    /* +0xC */ f32 radius;
};

#ifdef __cplusplus

/* A sphere a shape test builds on the stack (GUESS name): `HitSphere` with retail's out-of-line constructor, which
 * `ef/effect.cpp` emits first.  The body-shape tables stay `HitSphere` aggregates.  size: 0x10 */
struct PlSphere : public HitSphere {
    /* 0x800FA420 (0x30): constructs the position. */
    PlSphere() __attribute__((never_inline)) {}
};

/* A segment: the two endpoints `pl_segment_box_set` copies.  size: 0x18 (two 0xC-byte `VEC3::operator=` records) */
struct PlSegment : public nw4r::math::SEGMENT3 {
};

/* The box `pl_segment_box_set` fills: the endpoints, `end - start`, the midpoint (the sum scaled by the 0.5 at
 * .sdata2 0x8079A310) and the distance from `start` to it.  size: 0x34 (the last store is the f32 at +0x30) */
struct PlSegmentBox {
    /* +0x00 */ nw4r::math::VEC3 start;
    /* +0x0C */ nw4r::math::VEC3 end;
    /* +0x18 */ nw4r::math::VEC3 dir;
    /* +0x24 */ nw4r::math::VEC3 center;
    /* +0x30 */ f32 half_length;

    /* 0x8010294C (0x48): constructs the four vectors (emitted first in `ef/em_effect_ctrl.cpp`). */
    PlSegmentBox() __attribute__((never_inline)) {}
};

/* A capsule: two endpoints and the radius `pl_capsule_box_set` copies.  size: 0x1C (the f32 read at +0x18) */
struct PlCapsule : public nw4r::math::CAPSULE {
};

/* The box `pl_capsule_box_set` fills: the capsule, `end - start`, the midpoint and the bounding reach (the
 * distance from `start` to the midpoint plus the radius).  size: 0x38 (the last store is the f32 at +0x34) */
struct PlCapsuleBox : public nw4r::math::CAPSULE {
    /* +0x1C */ nw4r::math::VEC3 dir;
    /* +0x28 */ nw4r::math::VEC3 center;
    /* +0x34 */ f32 reach;

    /* 0x800FA378 (0x40): constructs the capsule and the two derived vectors (emitted first in `ef/effect.cpp`; retail
     * constructs the capsule through `CAPSULE::CAPSULE()`, the `nw4r::math::CAPSULE` constructor). */
    PlCapsuleBox() __attribute__((never_inline)) {}
};

extern "C" {

/* 0x8028F4B4 (0xA4): fills `box` from the segment's endpoints. */
void pl_segment_box_set(const PlSegment* seg, PlSegmentBox* box);

/* 0x8028F558 (0xC4): fills `box` from the capsule. */
void pl_capsule_box_set(PlCapsule* capsule, PlCapsuleBox* box);

/* 0x8028F44C (0x68): builds the first 0x24 bytes of box `b` from the two vectors at the head of `a` and their
 * difference (`enemy/em_common.cpp`'s reach test hands it a segment). */
void pl_box_set(struct PlBox* a, struct PlBox* b);

/* 0x80290524 (0x74): projects `point` onto `plane` into `out`. */
void pl_plane_project(nw4r::math::VEC3* point, nw4r::math::PLANE* plane, nw4r::math::VEC3* out);

/* The body push tests `Pl/pl_body_push.cpp` calls: 1 when the two shapes overlap, with the push that separates them. */
u32 pl_sphere_push(HitSphere* a, HitSphere* b, nw4r::math::VEC3* out);
u32 pl_capsule_capsule_push(PlCapsuleBox* a, PlCapsuleBox* b, nw4r::math::VEC3* push_a, nw4r::math::VEC3* push_b);
u32 pl_capsule_sphere_push(PlCapsuleBox* a, HitSphere* sphere, nw4r::math::VEC3* push_a,
                           nw4r::math::VEC3* push_sphere);

/* The hit contact tests `Pl/pl_hit.cpp` calls: 1 when the two shapes touch, with the contact point in `out`. */
s32 pl_sphere_contact(HitSphere* a, HitSphere* b, nw4r::math::VEC3* out);
s32 pl_capsule_capsule_contact(PlCapsuleBox* a, PlCapsuleBox* b, nw4r::math::VEC3* out);
s32 pl_capsule_sphere_contact(PlCapsuleBox* a, HitSphere* sphere, nw4r::math::VEC3* out);
s32 pl_sphere_capsule_contact(HitSphere* sphere, PlCapsuleBox* a, nw4r::math::VEC3* out);

/* The segment and triangle tests `Pl/pl_coll.cpp`'s land queries call. */
/* 0x8028F938 (0xE8): the point on segment `start`..`end` closest to `point`, into `out`; returns the squared distance. */
f32 pl_segment_closest_point(nw4r::math::VEC3* start, nw4r::math::VEC3* end, nw4r::math::VEC3* point,
                             nw4r::math::VEC3* out);
/* 0x80290840 (0x3E8): segment box `seg` against capsule `capsule`: 0 for a miss, else 1 with the crossing points. */
s32 pl_segment_capsule_hit(PlSegmentBox* seg, PlCapsuleBox* capsule, nw4r::math::VEC3* enter, nw4r::math::VEC3* exit);
/* 0x80290C28 (0xE0): where segment `seg` crosses `plane`, into `out`; 0 when it does not. */
u32 pl_segment_plane_hit(PlSegment* seg, nw4r::math::PLANE* plane, nw4r::math::VEC3* out);
/* 0x80290D08 (0x3B8): the point of triangle `a`/`b`/`c` closest to `point`, into `out`; returns its region. */
s32 pl_triangle_closest_point(nw4r::math::VEC3* point, nw4r::math::VEC3* a, nw4r::math::VEC3* b, nw4r::math::VEC3* c,
                              nw4r::math::VEC3* out);
/* 0x802910C0 (0x54): returns `in * scale`. */
nw4r::math::VEC3 vec3_scale_to(f32 scale, const nw4r::math::VEC3& in);

}

/* 0x8028F61C (0x50): 1 when `point` lies inside the sphere of `radius` around `center`. */
u32 hit_point_sphr(nw4r::math::VEC3* point, nw4r::math::VEC3* center, f32 radius);

#endif /* __cplusplus */

#endif /* MHTRI_PL_PL_HIT_SPHERE_H */
