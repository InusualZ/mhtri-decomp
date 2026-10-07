/*
 * nw4r/fn_805012C4.cpp - nw4r::math's matrix and bounding-box helpers and nw4r::ut's lists, binary-file check,
 *   character-stream reader, tag processor and locked-cache wrappers.
 * RANGE. .text 0x805012C4-0x80502828 (46 functions); .ctors 0x8056F3D8-0x8056F3DC; .data 0x8062FAC8-0x8062FAF0 (the
 *   two TagProcessorBase vtables); .bss 0x80760C78-0x80760C98 (the locked-cache state); .sdata2 0x8079D4C8-0x8079D4F0.
 * RANGE. The range is several nw4r source files in link order: math_types (0x805012C4-0x805016D0), math_geometry
 *   (-0x80501A4C), ut_list (-0x80501CE8), ut_LinkList (-0x80501E24), ut_binaryFileFormat (-0x80501E98),
 *   ut_CharStrmReader (-0x80501FA8), ut_TagProcessorBase (-0x80502678) and ut_LockedCache (-0x80502828, with the
 *   .ctors and .bss); each retail file has its own `.sdata2` pool, which one object cannot reproduce.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. GUESS from the nw4r source, read off the bodies: math's MTX33Identity, MTX34ToMTX33, MTX34Zero, MTX34Scale,
 *   MTX34Trans, MTX34RotAxisFIdx, MTX34RotXYZFIdx, MTX33ToMTX34, MTX44Identity, MTX44Copy (fn_805012C4..fn_8050168C),
 *   AABB::Set (fn_805016D0), Frustum::IntersectAABB_Ex (fn_805018A8) and the pair sFIdxCirclePair; ut's List_Init
 *   (MEMInitList) .. List_GetNth (fn_80501C9C), detail::LinkListImpl (fn_80501CE8..fn_80501DF8),
 *   IsValidBinaryFile (fn_80501E24), CharStrmReader::ReadNextChar* (fn_80501E98..fn_80501F48), LC::Enable ..
 *   LC::StoreData (fn_80502678..fn_8050280C, the tail calls in the SDK's LCLoadBlocks/LCLoadData/LCStoreBlocks/
 *   LCStoreData order), TagProcessorBase<char>/<wchar_t>'s constructor, destructor, Process and CalcRect
 *   (fn_80501FA8..fn_80502490, the map carrying the template manglings).  `mtx34_rotate_vec3` keeps the map's C name
 *   for nw4r's VEC3TransformNormal.
 * RESIDUALS. `.sdata2`: the object is 0x20 where splits.txt claims 0x28 (each retail file pads its own pool).
 * RESIDUALS. MTX34RotXYZFIdx 90.9 % (retail keeps the circle pair's address in r0 and schedules the
 *   table `lis` first); AABB::Set 89.4 % (row maxima colour f1/f3 where retail has f2/f3); Frustum::IntersectAABB_Ex
 *   99.4 % (VEC3Dot's register colouring); List_Remove 97.2 % (r5/r6 swap).
 * SHAPES. The paired-single bodies are C with one inline-asm block over `register` locals; GetChar/StepStrm pun
 *   mCharStrm through a reference so each access reloads it; Lock tests `isEnabled` true-first.
 */

#include "nw4r/math.h"
#include "nw4r/fn_805012C4.h"
#include "nw4r/TextWriterBase.h"
#include "NAND/nand.h"
#include "OS/LCEnable.h"
#include "OS/OSVReport.h"
#include "OS/OSThread.h"
#include "MTX/PSMTXRotAxisRad.h"
#include "unsplit/OS.h"
#include "nw4r/math_triangular.h"
#include "OS/OSFastCast.h"


/* ---------------------------------------------------------------------------------------------------------------
 * math_types
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace math {

/* The circle size in index units, as the paired-single pair the rotation code compares against. */
static const f32 sFIdxCirclePair[2] = {65536.0f, 65536.0f};

/* 0x805012C4 (0x24): sets a 3x3 matrix to identity. */
MTX33* MTX33Identity(register MTX33* pOut) {
    register f32 zero = 0.0f;
    register f32 one = 1.0f;
    register f32 oneZero;

    asm {
        psq_st zero, 8(pOut), 0, 0
        ps_merge00 oneZero, one, zero
        psq_st zero, 24(pOut), 0, 0
        psq_st oneZero, 0(pOut), 0, 0
        psq_st oneZero, 16(pOut), 0, 0
        stfs one, 32(pOut)
    }
    return pOut;
}

/* 0x805012E8 (0x34): copies the rotation part of a 3x4 matrix into a 3x3 one. */
MTX33* MTX34ToMTX33(register MTX33* pOut, register const MTX34* pM) {
    register f32 row0a;
    register f32 row0b;
    register f32 row1a;
    register f32 row1b;
    register f32 row2a;
    register f32 row2b;

    asm {
        psq_l row0a, 0(pM), 0, 0
        psq_l row0b, 8(pM), 0, 0
        psq_l row1a, 16(pM), 0, 0
        psq_l row1b, 24(pM), 0, 0
        psq_l row2a, 32(pM), 0, 0
        psq_l row2b, 40(pM), 0, 0
        psq_st row0a, 0(pOut), 0, 0
        psq_st row0b, 8(pOut), 1, 0
        psq_st row1a, 12(pOut), 0, 0
        psq_st row1b, 20(pOut), 1, 0
        psq_st row2a, 24(pOut), 0, 0
        psq_st row2b, 32(pOut), 1, 0
    }
    return pOut;
}

/* 0x8050131C (0x20): zeroes a 3x4 matrix. */
MTX34* MTX34Zero(register MTX34* pOut) {
    register f32 zero = 0.0f;

    asm {
        psq_st zero, 0(pOut), 0, 0
        psq_st zero, 8(pOut), 0, 0
        psq_st zero, 16(pOut), 0, 0
        psq_st zero, 24(pOut), 0, 0
        psq_st zero, 32(pOut), 0, 0
        psq_st zero, 40(pOut), 0, 0
    }
    return pOut;
}

/* 0x8050133C (0x54): scales the columns of a 3x4 matrix by a vector (translation untouched). */
MTX34* MTX34Scale(register MTX34* pOut, register const MTX34* pM, register const VEC3* pS) {
    register f32 xy;
    register f32 z1;
    register f32 row0a;
    register f32 row0b;
    register f32 row1a;
    register f32 row1b;
    register f32 row2a;
    register f32 row2b;

    asm {
        psq_l xy, 0(pS), 0, 0
        psq_l row0a, 0(pM), 0, 0
        psq_l row1a, 16(pM), 0, 0
        psq_l row2a, 32(pM), 0, 0
        ps_mul row0a, row0a, xy
        ps_mul row1a, row1a, xy
        psq_l z1, 8(pS), 1, 0
        ps_mul row2a, row2a, xy
        psq_l row0b, 8(pM), 0, 0
        psq_l row1b, 24(pM), 0, 0
        psq_l row2b, 40(pM), 0, 0
        ps_mul row0b, row0b, z1
        psq_st row0a, 0(pOut), 0, 0
        ps_mul row1b, row1b, z1
        ps_mul row2b, row2b, z1
        psq_st row1a, 16(pOut), 0, 0
        psq_st row0b, 8(pOut), 0, 0
        psq_st row1b, 24(pOut), 0, 0
        psq_st row2a, 32(pOut), 0, 0
        psq_st row2b, 40(pOut), 0, 0
    }
    return pOut;
}

/* 0x80501390 (0x6C): copies a 3x4 matrix with its translation moved by the matrix applied to a vector. */
MTX34* MTX34Trans(register MTX34* pOut, register const MTX34* pM, register const VEC3* pT) {
    register f32 xy;
    register f32 z1;
    register f32 row0a;
    register f32 row0b;
    register f32 row1a;
    register f32 row1b;
    register f32 row2a;
    register f32 row2b;
    register f32 prod;
    register f32 sum;
    register f32 trans;

    asm {
        psq_l row0a, 0(pM), 0, 0
        psq_l xy, 0(pT), 0, 0
        psq_l row0b, 8(pM), 0, 0
        psq_l row1a, 16(pM), 0, 0
        ps_mul prod, row0a, xy
        psq_l z1, 8(pT), 1, 0
        psq_l row1b, 24(pM), 0, 0
        ps_madd sum, row0b, z1, prod
        psq_l row2a, 32(pM), 0, 0
        ps_mul prod, row1a, xy
        psq_l row2b, 40(pM), 0, 0
        psq_st row0b, 8(pOut), 0, 0
        ps_sum0 trans, sum, trans, sum
        ps_madd sum, row1b, z1, prod
        psq_st row1b, 24(pOut), 0, 0
        ps_mul prod, row2a, xy
        psq_st trans, 12(pOut), 1, 0
        ps_sum0 trans, sum, trans, sum
        ps_madd sum, row2b, z1, prod
        psq_st row2b, 40(pOut), 0, 0
        psq_st trans, 28(pOut), 1, 0
        ps_sum0 trans, sum, trans, sum
        psq_st row0a, 0(pOut), 0, 0
        psq_st row1a, 16(pOut), 0, 0
        psq_st row2a, 32(pOut), 0, 0
        psq_st trans, 44(pOut), 1, 0
    }
    return pOut;
}

/* 0x805013FC (0x38): builds a rotation about an axis by an angle index. */
MTX34* MTX34RotAxisFIdx(MTX34* pOut, const VEC3* pAxis, f32 fIdx) {
    PSMTXRotAxisRad(pOut, pAxis, 0.024543693f * fIdx);
    return pOut;
}

/* 0x80501434 (0x160): builds the rotation Rz * Ry * Rx from three angle indices with paired-single table
 * lookups. */
MTX34* MTX34RotXYZFIdx(register MTX34* pOut, register f32 fx, register f32 fy, register f32 fz) {
    u32 idx;
    register u32* pIdx = &idx;
    register const f32* pCircle = sFIdxCirclePair;
    register const SinCosSample* tbl = sSinCosTbl;

    asm {
        psq_lx f0, 0, pCircle, 0, 0
        ps_merge00 f6, fx, fy
        ps_merge00 f0, f0, f0
        ps_abs f4, f6
        ps_neg f1, f0
        ps_sub f2, f0, f0
        ps_cmpu0 cr0, f4, f0
        ble _reduceY
    _loopX:
        ps_sum0 f4, f4, f4, f1
        ps_cmpu0 cr0, f4, f0
        bgt _loopX
    _reduceY:
        ps_cmpu1 cr0, f4, f0
        ble _convertXY
        ps_merge10 f4, f4, f4
    _loopY:
        ps_sum0 f4, f4, f4, f1
        ps_cmpu0 cr0, f4, f0
        bgt _loopY
        ps_merge10 f4, f4, f4
    _convertXY:
        psq_st f4, 0(pIdx), 0, OS_FASTCAST_U16
        psq_l f7, 0(pIdx), 0, OS_FASTCAST_U16
        fabs f5, fz
        lwz r0, 0(pIdx)
        fcmpu cr0, f5, f0
        ble _convertZ
    _loopZ:
        fsubs f5, f5, f0
        fcmpu cr0, f5, f0
        bgt _loopZ
    _convertZ:
        psq_st f5, 0(pIdx), 1, OS_FASTCAST_U16
        rlwinm r5, r0, 20, 20, 27
        add r5, tbl, r5
        ps_sub f7, f4, f7
        psq_l f4, 0(r5), 0, 0
        rlwinm r6, r0, 4, 20, 27
        psq_l f8, 8(r5), 0, 0
        ps_cmpu0 cr0, f6, f2
        add r6, tbl, r6
        ps_madds0 f0, f8, f7, f4
        psq_l f4, 0(r6), 0, 0
        psq_l f8, 8(r6), 0, 0
        lhz r0, 0(pIdx)
        bge _signY
        ps_neg f9, f0
        ps_merge01 f0, f9, f0
    _signY:
        ps_madds1 f1, f8, f7, f4
        psq_l f7, 0(pIdx), 1, OS_FASTCAST_U16
        rlwinm r0, r0, 4, 20, 27
        ps_cmpu1 cr0, f6, f2
        add r5, tbl, r0
        fsubs f7, f5, f7
        psq_l f4, 0(r5), 0, 0
        psq_l f8, 8(r5), 0, 0
        bge _signZ
        ps_neg f9, f1
        ps_merge01 f1, f9, f1
    _signZ:
        fcmpu cr0, fz, f2
        ps_madds0 f2, f8, f7, f4
        bge _build
        ps_neg f9, f2
        ps_merge01 f2, f9, f2
    _build:
        ps_neg f3, f0
        ps_muls1 f5, f2, f1
        ps_sub f7, f0, f0
        ps_merge10 f3, f3, f0
        ps_merge10 f6, f5, f5
        ps_muls0 f4, f0, f2
        psq_st f7, 44(pOut), 1, 0
        ps_muls0 f8, f3, f2
        psq_st f6, 0(pOut), 1, 0
        ps_muls1 f6, f0, f2
        ps_muls1 f2, f3, f2
        ps_madds0 f6, f6, f1, f8
        ps_neg f2, f2
        psq_st f6, 4(pOut), 0, 0
        ps_merge00 f6, f7, f5
        psq_st f6, 12(pOut), 0, 0
        ps_madds0 f6, f4, f1, f2
        psq_st f6, 20(pOut), 0, 0
        ps_neg f6, f1
        ps_merge00 f6, f7, f6
        psq_st f6, 28(pOut), 0, 0
        ps_muls1 f6, f0, f1
        psq_st f6, 36(pOut), 0, 0
    }
    return pOut;
}

/* 0x80501594 (0x34): copies a 3x3 matrix into the rotation part of a 3x4 one (translation untouched). */
MTX34* MTX33ToMTX34(register MTX34* pOut, register const MTX33* pM) {
    register f32 row0a;
    register f32 row1a;
    register f32 row2a;
    register f32 row0b;
    register f32 row1b;
    register f32 row2b;

    asm {
        psq_l row0a, 0(pM), 0, 0
        psq_l row1a, 12(pM), 0, 0
        psq_l row2a, 24(pM), 0, 0
        lfs row0b, 8(pM)
        lfs row1b, 20(pM)
        lfs row2b, 32(pM)
        psq_st row0a, 0(pOut), 0, 0
        psq_st row1a, 16(pOut), 0, 0
        psq_st row2a, 32(pOut), 0, 0
        stfs row0b, 8(pOut)
        stfs row1b, 24(pOut)
        stfs row2b, 40(pOut)
    }
    return pOut;
}

}  // namespace math
}  // namespace nw4r

/* 0x805015C8 (0x90): applies the rotation part of a 3x4 matrix to a vector (nw4r's VEC3TransformNormal). */
extern "C" void mtx34_rotate_vec3(nw4r::math::VEC3* out, const nw4r::math::MTX34* mtx, const nw4r::math::VEC3* v) {
    nw4r::math::VEC3 tmp;

    tmp.x = mtx->m[0][0] * v->x + mtx->m[0][1] * v->y + mtx->m[0][2] * v->z;
    tmp.y = mtx->m[1][0] * v->x + mtx->m[1][1] * v->y + mtx->m[1][2] * v->z;
    tmp.z = mtx->m[2][0] * v->x + mtx->m[2][1] * v->y + mtx->m[2][2] * v->z;
    out->x = tmp.x;
    out->y = tmp.y;
    out->z = tmp.z;
}

namespace nw4r {
namespace math {

/* 0x80501658 (0x34): sets a 4x4 matrix to identity. */
MTX44* MTX44Identity(register MTX44* pOut) {
    register f32 zero = 0.0f;
    register f32 one = 1.0f;
    register f32 zeroOne;
    register f32 oneZero;

    asm {
        psq_st zero, 8(pOut), 0, 0
        ps_merge01 zeroOne, zero, one
        ps_merge10 oneZero, one, zero
        psq_st zero, 24(pOut), 0, 0
        psq_st zero, 32(pOut), 0, 0
        psq_st zeroOne, 16(pOut), 0, 0
        psq_st oneZero, 0(pOut), 0, 0
        psq_st oneZero, 40(pOut), 0, 0
        psq_st zero, 48(pOut), 0, 0
        psq_st zeroOne, 56(pOut), 0, 0
    }
    return pOut;
}

/* 0x8050168C (0x44): copies a 4x4 matrix. */
MTX44* MTX44Copy(register MTX44* pOut, register const MTX44* pM) {
    register f32 v0;
    register f32 v1;
    register f32 v2;
    register f32 v3;
    register f32 v4;
    register f32 v5;
    register f32 v6;
    register f32 v7;

    asm {
        psq_l v0, 0(pM), 0, 0
        psq_l v1, 8(pM), 0, 0
        psq_l v2, 16(pM), 0, 0
        psq_l v3, 24(pM), 0, 0
        psq_l v4, 32(pM), 0, 0
        psq_l v5, 40(pM), 0, 0
        psq_l v6, 48(pM), 0, 0
        psq_l v7, 56(pM), 0, 0
        psq_st v0, 0(pOut), 0, 0
        psq_st v1, 8(pOut), 0, 0
        psq_st v2, 16(pOut), 0, 0
        psq_st v3, 24(pOut), 0, 0
        psq_st v4, 32(pOut), 0, 0
        psq_st v5, 40(pOut), 0, 0
        psq_st v6, 48(pOut), 0, 0
        psq_st v7, 56(pOut), 0, 0
    }
    return pOut;
}

}  // namespace math
}  // namespace nw4r

/* ---------------------------------------------------------------------------------------------------------------
 * math_geometry
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace math {

/* The dot product of two vectors in paired singles. */
inline f32 VEC3Dot(register const VEC3* a, register const VEC3* b) {
    register f32 res;
    register f32 yz;
    register f32 ax;
    register f32 byz;
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
    f32 minX, maxX, minY, maxY, minZ, maxZ;
    f32 ya, yb, za, zb;

    minX = mtx->m[0][3] + mtx->m[0][0] * box->min.x;
    maxX = mtx->m[0][3] + mtx->m[0][0] * box->max.x;
    ya = mtx->m[0][1] * box->min.y;
    yb = mtx->m[0][1] * box->max.y;
    za = mtx->m[0][2] * box->min.z;
    zb = mtx->m[0][2] * box->max.z;
    if (minX > maxX) {
        f32 t = minX;
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
        f32 t = minY;
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
        f32 t = minZ;
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

/* ---------------------------------------------------------------------------------------------------------------
 * ut_list
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace ut {

/* The link record inside `object`, `list->offset` bytes in. */
/* untyped: caller-owned payload - the list holds elements of any type */
static inline Link* GetLink(const List* list, const void* object) {
    return reinterpret_cast<Link*>(reinterpret_cast<u32>(object) + list->offset);
}

/* Makes `object` the only element of an empty list. */
/* untyped: caller-owned payload - the list holds elements of any type */
static inline void SetFirstObject(List* list, void* object) {
    Link* link = GetLink(list, object);

    link->nextObject = NULL;
    link->prevObject = NULL;
    list->headObject = object;
    list->tailObject = object;
    list->numObjects++;
}

/* 0x80501A4C (0x18): empties a list whose elements keep their link `offset` bytes in. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...) */
void List_Init(List* list, u16 offset) {
    list->headObject = NULL;
    list->tailObject = NULL;
    list->numObjects = 0;
    list->offset = offset;
}

/* 0x80501A64 (0x70): appends `object` at the tail. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Append(List* list, void* object) {
    if (list->headObject == NULL) {
        SetFirstObject(list, object);
    } else {
        Link* link = GetLink(list, object);

        link->prevObject = list->tailObject;
        link->nextObject = NULL;
        GetLink(list, list->tailObject)->nextObject = object;
        list->tailObject = object;
        list->numObjects++;
    }
}

/* Prepends `object` at the head. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
static inline void List_Prepend(List* list, void* object) {
    if (list->headObject == NULL) {
        SetFirstObject(list, object);
    } else {
        Link* link = GetLink(list, object);

        link->prevObject = NULL;
        link->nextObject = list->headObject;
        GetLink(list, list->headObject)->prevObject = object;
        list->headObject = object;
        list->numObjects++;
    }
}

/* 0x80501AD4 (0x120): links `object` in front of `target`, or at the tail when `target` is NULL. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Insert(List* list, void* target, void* object) {
    if (target == NULL) {
        List_Append(list, object);
    } else if (target == list->headObject) {
        List_Prepend(list, object);
    } else {
        Link* link = GetLink(list, object);
        void* prev = GetLink(list, target)->prevObject;
        Link* prevLink = GetLink(list, prev);

        link->prevObject = prev;
        link->nextObject = target;
        prevLink->nextObject = object;
        GetLink(list, target)->prevObject = object;
        list->numObjects++;
    }
}

/* 0x80501BF4 (0x6C): unlinks `object`. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void List_Remove(List* list, void* object) {
    Link* link = GetLink(list, object);

    if (link->prevObject == NULL) {
        list->headObject = link->nextObject;
    } else {
        GetLink(list, link->prevObject)->nextObject = link->nextObject;
    }
    if (link->nextObject == NULL) {
        list->tailObject = link->prevObject;
    } else {
        GetLink(list, link->nextObject)->prevObject = link->prevObject;
    }
    link->prevObject = NULL;
    link->nextObject = NULL;
    list->numObjects--;
}

/* 0x80501C60 (0x20): the element after `object`, or the head when `object` is NULL. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNext(const List* list, const void* object) {
    if (object == NULL) {
        return list->headObject;
    }
    return GetLink(list, object)->nextObject;
}

/* 0x80501C80 (0x1C): the element before `object`, or the tail when `object` is NULL. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetPrev(const List* list, const void* object) {
    if (object == NULL) {
        return list->tailObject;
    }
    return GetLink(list, object)->prevObject;
}

/* 0x80501C9C (0x4C): the element at `index`, or NULL past the end. */
/* free: SDK C struct - nw4r::ut's List API is namespace-scope (List_Init__Q24nw4r2utF...); untyped: caller-owned payload - the list holds elements of any type */
void* List_GetNth(const List* list, u16 index) {
    int count = 0;
    void* object = NULL;

    while ((object = List_GetNext(list, object)) != NULL) {
        if (index == count) {
            return object;
        }
        count++;
    }
    return NULL;
}

}  // namespace ut
}  // namespace nw4r

/* ---------------------------------------------------------------------------------------------------------------
 * ut_LinkList
 * ------------------------------------------------------------------------------------------------------------ */

namespace nw4r {
namespace ut {
namespace detail {

/* Unlinks `node` and returns the node after it. */
inline LinkListNode* LinkListImpl::Erase(LinkListNode* node) {
    LinkListNode* next = node->mNext;
    LinkListNode* prev = node->mPrev;

    next->mPrev = prev;
    prev->mNext = next;
    mSize--;
    node->mNext = NULL;
    node->mPrev = NULL;
    return next;
}

/* Unlinks every node from `first` up to `last`, returning `last`. */
inline LinkListImpl::Iterator LinkListImpl::Erase(Iterator first, Iterator last) {
    LinkListNode* node = first.mPointer;
    LinkListNode* end = last.mPointer;
    LinkListNode* next;

    while (node != end) {
        next = node->mNext;
        Erase(node);
        node = next;
    }
    return last;
}

/* 0x80501CE8 (0x84): unlinks every node, then frees the list when asked to. */
LinkListImpl::~LinkListImpl() {
    Clear();
}

/* 0x80501D6C (0x48): unlinks the node at `it` and returns the position after it. */
LinkListImpl::Iterator LinkListImpl::Erase(Iterator it) {
    Iterator next = Iterator(it.mPointer->mNext);

    return Erase(it, next);
}

/* 0x80501DB4 (0x44): unlinks every node. */
void LinkListImpl::Clear() {
    Erase(GetBeginIter(), GetEndIter());
}

/* 0x80501DF8 (0x2C): links `node` in front of `it` and returns its position. */
LinkListImpl::Iterator LinkListImpl::Insert(Iterator it, LinkListNode* node) {
    LinkListNode* next = it.mPointer;
    LinkListNode* prev = next->mPrev;

    node->mNext = next;
    node->mPrev = prev;
    next->mPrev = node;
    prev->mNext = node;
    mSize++;
    return Iterator(node);
}

}  // namespace detail

/* ---------------------------------------------------------------------------------------------------------------
 * ut_binaryFileFormat
 * ------------------------------------------------------------------------------------------------------------ */

/* 0x80501E24 (0x74): checks a resource file's signature, byte order, version, size and block count. */
bool IsValidBinaryFile(const BinaryFileHeader* header, u32 signature, u16 version, u16 minBlocks) {
    if (header->signature != signature) {
        return false;
    }
    if (header->byteOrder != 0xFEFF) {
        return false;
    }
    if (header->version != version) {
        return false;
    }
    if (header->fileSize < sizeof(BinaryFileHeader) + sizeof(BinaryBlockHeader) * minBlocks) {
        return false;
    }
    if (header->dataBlocks < minBlocks) {
        return false;
    }
    return true;
}

/* ---------------------------------------------------------------------------------------------------------------
 * ut_CharStrmReader
 * ------------------------------------------------------------------------------------------------------------ */

/* Whether `c` opens a two-byte Shift-JIS character. */
static inline bool IsSJISLeadByte(u8 c) {
    return (c >= 0x81 && c < 0xA0) || c >= 0xE0;
}

/* 0x80501E98 (0x78): decodes one UTF-8 character (up to three bytes). */
u16 CharStrmReader::ReadNextCharUTF8() {
    u16 code;

    if (!(GetChar<u8>(0) & 0x80)) {
        code = GetChar<u8>(0);
        StepStrm<u8>(1);
    } else if ((GetChar<u8>(0) & 0xE0) == 0xC0) {
        code = ((GetChar<u8>(0) & 0x1F) << 6) | (GetChar<u8>(1) & 0x3F);
        StepStrm<u8>(2);
    } else {
        code = ((GetChar<u8>(0) & 0x1F) << 12) | ((GetChar<u8>(1) & 0x3F) << 6) | (GetChar<u8>(2) & 0x3F);
        StepStrm<u8>(3);
    }
    return code;
}

/* 0x80501F10 (0x1C): reads one UTF-16 code unit. */
u16 CharStrmReader::ReadNextCharUTF16() {
    u16 code = GetChar<u16>(0);

    StepStrm<u16>(1);
    return code;
}

/* 0x80501F2C (0x1C): reads one CP1252 byte. */
u16 CharStrmReader::ReadNextCharCP1252() {
    u16 code = GetChar<u8>(0);

    StepStrm<u8>(1);
    return code;
}

/* 0x80501F48 (0x60): decodes one Shift-JIS character (one or two bytes). */
u16 CharStrmReader::ReadNextCharSJIS() {
    u16 code;

    if (IsSJISLeadByte(GetChar<u8>(0))) {
        code = (GetChar<u8>(0) << 8) | GetChar<u8>(1);
        StepStrm<u8>(2);
    } else {
        code = GetChar<u8>(0);
        StepStrm<u8>(1);
    }
    return code;
}

/* ---------------------------------------------------------------------------------------------------------------
 * ut_TagProcessorBase
 * ------------------------------------------------------------------------------------------------------------ */

/* Makes a tag processor. */
template <typename T> TagProcessorBase<T>::TagProcessorBase() {}

/* Destroys a tag processor. */
template <typename T> TagProcessorBase<T>::~TagProcessorBase() {}

/* Acts on a line feed or a tab like Process and returns the rectangle the cursor moved through. */
template <typename T> Operation TagProcessorBase<T>::CalcRect(Rect* rect, u16 code, PrintContext<T>* context) {
    switch (code) {
    case '\n': {
        TextWriterBase<T>& writer = *context->writer;

        rect->right = writer.GetCursorX();
        rect->top = writer.GetCursorY();
        ProcessLinefeed(context);
        rect->left = writer.GetCursorX();
        rect->bottom = writer.GetCursorY() + context->writer->GetFontHeight();
        rect->Normalize();
        return OPERATION_NEXT_LINE;
    }
    case '\t': {
        TextWriterBase<T>& writer = *context->writer;

        rect->left = writer.GetCursorX();
        ProcessTab(context);
        rect->right = writer.GetCursorX();
        rect->top = writer.GetCursorY();
        rect->bottom = rect->top + writer.GetFontHeight();
        rect->Normalize();
        return OPERATION_NO_CHAR_SPACE;
    }
    default:
        return OPERATION_DEFAULT;
    }
}

/* Acts on a line feed or a tab; anything else is left to the writer. */
template <typename T> Operation TagProcessorBase<T>::Process(u16 code, PrintContext<T>* context) {
    switch (code) {
    case '\n':
        ProcessLinefeed(context);
        return OPERATION_NEXT_LINE;
    case '\t':
        ProcessTab(context);
        return OPERATION_NO_CHAR_SPACE;
    default:
        return OPERATION_DEFAULT;
    }
}

template class TagProcessorBase<char>;
template class TagProcessorBase<wchar_t>;

/* ---------------------------------------------------------------------------------------------------------------
 * ut_LockedCache
 * ------------------------------------------------------------------------------------------------------------ */

namespace LC {

/* The locked cache's state: whether it is enabled and the mutex that guards it. size: 0x1C */
struct LCImpl {
    LCImpl() : isEnabled(false) {
        OSInitMutex(&mutex);
    }

    void Lock() {
        OSLockMutex(&mutex);
    }

    void Unlock() {
        OSUnlockMutex(&mutex);
    }

    /* +0x00 */ bool isEnabled;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ OSMutex mutex;
};

/* Holds the locked cache's mutex for the scope it lives in. size: 0x4 */
class AutoLock {
public:
    AutoLock(LCImpl& impl) : mMutex(&impl.mutex) {
        OSLockMutex(mMutex);
    }

    ~AutoLock() {
        OSUnlockMutex(mMutex);
    }

    /* +0x0 */ OSMutex* mMutex;
};

static LCImpl sLCImpl;

/* Waits until the cache's DMA queue is empty. */
static inline void QueueWait() {
    while (LCQueueLength() != 0) {
        OSYieldThread();
    }
}

/* 0x80502678 (0x60): enables the locked cache once. */
void Enable() {
    AutoLock lock(sLCImpl);

    if (!sLCImpl.isEnabled) {
        LCEnable();
        sLCImpl.isEnabled = true;
    }
}

/* 0x805026D8 (0x78): drains the DMA queue and disables the locked cache. */
void Disable() {
    AutoLock lock(sLCImpl);

    if (sLCImpl.isEnabled) {
        QueueWait();
        LCDisable();
        sLCImpl.isEnabled = false;
    }
}

/* 0x80502750 (0x70): takes the cache for the caller; false (and released) when it is not enabled. */
bool Lock() {
    sLCImpl.Lock();
    if (sLCImpl.isEnabled) {
        QueueWait();
        return true;
    } else {
        sLCImpl.Unlock();
        return false;
    }
}

/* 0x805027C0 (0x40): drains the DMA queue and releases the cache. */
void Unlock() {
    QueueWait();
    sLCImpl.Unlock();
}

/* 0x80502800 (0x4): DMAs `blocks` 32-byte blocks into the locked cache. */
/* untyped: byte range */
void LoadBlocks(void* dst, void* src, u32 blocks) {
    LCLoadBlocks(dst, src, blocks);
}

/* 0x80502804 (0x4): DMAs `size` bytes into the locked cache. */
/* untyped: byte range */
void LoadData(void* dst, void* src, u32 size) {
    LCLoadData(dst, src, size);
}

/* 0x80502808 (0x4): DMAs `blocks` 32-byte blocks out of the locked cache. */
/* untyped: byte range */
void StoreBlocks(void* dst, void* src, u32 blocks) {
    LCStoreBlocks(dst, src, blocks);
}

/* 0x8050280C (0x4): DMAs `size` bytes out of the locked cache. */
/* untyped: byte range */
void StoreData(void* dst, void* src, u32 size) {
    LCStoreData(dst, src, size);
}

}  // namespace LC

}  // namespace ut
}  // namespace nw4r
