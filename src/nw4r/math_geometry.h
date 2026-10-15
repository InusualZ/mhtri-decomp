/*
 * nw4r/math_geometry.h - nw4r::math's plane, axis-aligned box and frustum, whose members `nw4r/math_geometry.cpp`
 *   defines.
 */
#ifndef NW4R_MATH_GEOMETRY_H
#define NW4R_MATH_GEOMETRY_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
namespace nw4r {
namespace math {

/* A plane n.p + d = 0. size: 0x10 */
struct PLANE {
    f32 Test(const VEC3& p) const;

    /* +0x0 */ VEC3 N;
    /* +0xC */ f32 d;
};

/* An axis-aligned box. size: 0x18 */
struct AABB {
    void Set(const AABB* box, const MTX34* mtx);

    /* +0x00 */ VEC3 min;
    /* +0x0C */ VEC3 max;
};

/* A view frustum: its camera matrix, side planes, depth range, bounding box and the six planes the box tests
 * read. size: 0xF0 */
class Frustum {
public:
    int IntersectAABB_Ex(const AABB* box) const;

    /* +0x00 */ MTX34 cam;
    /* +0x30 */ u8 pad_0x30[0x48];
    /* +0x78 */ AABB box;
    /* +0x90 */ PLANE planes[6];
};

}  // namespace math
}  // namespace nw4r
#endif

#endif
