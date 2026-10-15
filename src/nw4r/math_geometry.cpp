/*
 * nw4r/math_geometry.cpp - nw4r::math's axis-aligned box transform and the frustum-box test.
 * RANGE. .text 0x805016D0-0x80501A4C (2 functions); .sdata2 0x8079D4E0-0x8079D4E8 (0.0f and the pool's padding).
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's math_geometry.cpp (AABB, Frustum). GUESS from the nw4r source, read off the bodies:
 *   AABB::Set (fn_805016D0) and Frustum::IntersectAABB_Ex (fn_805018A8).
 * RESIDUALS. AABB::Set: the first row's maximum and swap temporary still colour apart from retail's (f2 for maxX,
 *   f1 for t); the body as products swapped before the add, `max < min` and swapping max first are worse.
 * SHAPES. AABB::Set declares its locals one per line in the order a 200-sample permutation search ranked best,
 *   with the swap temporary `t` at function scope; VEC3Dot declares byz before ax, which colours
 *   Frustum::IntersectAABB_Ex's two dot products as retail.
 */

#include "nw4r/math.h"
#include "nw4r/math_geometry.h"

namespace nw4r {
namespace math {

/* The dot product of two vectors in paired singles. */
inline f32 VEC3Dot(register const VEC3* a, register const VEC3* b) {
    register f32 res;
    register f32 yz;
    register f32 byz;
    register f32 ax;
    register f32 bx;

    asm {
        psq_l yz, 4(a), 0, 0
        psq_l byz, 4(b), 0, 0
        psq_l ax, 0(a), 1, 0
        ps_mul yz, yz, byz
        psq_l bx, 0(b), 1, 0
        ps_madd byz, ax, bx, yz
        ps_sum0 res, byz, yz, yz
    }
    return res;
}

/* The signed distance of `p` from the plane. */
inline f32 PLANE::Test(const VEC3& p) const {
    return d + VEC3Dot(&N, &p);
}

/* 0x805016D0 (0x1D8): sets this box to the axis-aligned bounds of `box` transformed by `mtx`. */
void AABB::Set(const AABB* box, const MTX34* mtx) {
    f32 minX;
    f32 ya;
    f32 minY;
    f32 minZ;
    f32 maxX;
    f32 za;
    f32 maxY;
    f32 maxZ;
    f32 yb;
    f32 t;
    f32 zb;

    minX = mtx->m[0][3] + mtx->m[0][0] * box->min.x;
    maxX = mtx->m[0][3] + mtx->m[0][0] * box->max.x;
    ya = mtx->m[0][1] * box->min.y;
    yb = mtx->m[0][1] * box->max.y;
    za = mtx->m[0][2] * box->min.z;
    zb = mtx->m[0][2] * box->max.z;
    if (minX > maxX) {
        t = minX;
        minX = maxX;
        maxX = t;
    }
    if (ya < yb) {
        minX += ya;
        maxX += yb;
    } else {
        minX += yb;
        maxX += ya;
    }
    if (za < zb) {
        minX += za;
        maxX += zb;
    } else {
        minX += zb;
        maxX += za;
    }

    minY = mtx->m[1][3] + mtx->m[1][0] * box->min.x;
    maxY = mtx->m[1][3] + mtx->m[1][0] * box->max.x;
    ya = mtx->m[1][1] * box->min.y;
    yb = mtx->m[1][1] * box->max.y;
    za = mtx->m[1][2] * box->min.z;
    zb = mtx->m[1][2] * box->max.z;
    if (minY > maxY) {
        t = minY;
        minY = maxY;
        maxY = t;
    }
    if (ya < yb) {
        minY += ya;
        maxY += yb;
    } else {
        minY += yb;
        maxY += ya;
    }
    if (za < zb) {
        minY += za;
        maxY += zb;
    } else {
        minY += zb;
        maxY += za;
    }

    minZ = mtx->m[2][3] + mtx->m[2][0] * box->min.x;
    maxZ = mtx->m[2][3] + mtx->m[2][0] * box->max.x;
    ya = mtx->m[2][1] * box->min.y;
    yb = mtx->m[2][1] * box->max.y;
    za = mtx->m[2][2] * box->min.z;
    zb = mtx->m[2][2] * box->max.z;
    if (minZ > maxZ) {
        t = minZ;
        minZ = maxZ;
        maxZ = t;
    }
    if (ya < yb) {
        minZ += ya;
        maxZ += yb;
    } else {
        minZ += yb;
        maxZ += ya;
    }
    if (za < zb) {
        minZ += za;
        maxZ += zb;
    } else {
        minZ += zb;
        maxZ += za;
    }

    min.x = minX;
    min.y = minY;
    min.z = minZ;
    max.x = maxX;
    max.y = maxY;
    max.z = maxZ;
}

/* Whether two axis-aligned boxes overlap. */
static inline bool IntersectAABB(const AABB* a, const AABB* b) {
    if (a->min.x > b->max.x || b->min.x > a->max.x || a->min.y > b->max.y || b->min.y > a->max.y ||
        a->min.z > b->max.z || b->min.z > a->max.z) {
        return false;
    }
    return true;
}

/* 0x805018A8 (0x1A4): classifies `box` against the frustum: 0 outside, 1 inside, 2 crossing a plane. */
int Frustum::IntersectAABB_Ex(const AABB* box) const {
    int result;
    int i;

    if (!IntersectAABB(box, &this->box)) {
        return 0;
    }

    result = 1;
    for (i = 0; i < 6; i++) {
        VEC3 nearPt;
        VEC3 farPt;

        if (planes[i].N.x >= 0.0f) {
            nearPt.x = box->min.x;
            farPt.x = box->max.x;
        } else {
            nearPt.x = box->max.x;
            farPt.x = box->min.x;
        }
        if (planes[i].N.y >= 0.0f) {
            nearPt.y = box->min.y;
            farPt.y = box->max.y;
        } else {
            nearPt.y = box->max.y;
            farPt.y = box->min.y;
        }
        if (planes[i].N.z >= 0.0f) {
            nearPt.z = box->min.z;
            farPt.z = box->max.z;
        } else {
            nearPt.z = box->max.z;
            farPt.z = box->min.z;
        }

        if (planes[i].Test(nearPt) > 0.0f) {
            return 0;
        }
        if (planes[i].Test(farPt) > 0.0f) {
            result = 2;
        }
    }
    return result;
}

}  // namespace math
}  // namespace nw4r
