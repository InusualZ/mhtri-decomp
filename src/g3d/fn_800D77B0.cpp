/*
 * g3d/fn_800D77B0.cpp - the single-matrix node transform (by `pRec->flags`) and the matrix slot's sign-bit test.
 * RANGE. .text 0x800D77B0-0x800D79B4 (2 functions); extab, extabindex.  Between `g3d/g3d_xsi.cpp` and
 *   `g3d/g3d_basic.cpp`, with no `__FILE__` string of its own; fn_800D77B0 is the per-node world-matrix builder
 *   `g3d/g3d_calcworld.cpp`'s fn_800737CC calls, and its multi-matrix sibling fn_800D7D24 is in `g3d/g3d_basic.cpp`.
 * NAMES. Map stems (no name evidence); `pRec` is a private view of `g3d/g3d_calcworld.cpp`'s `NodeMtxRec` (the
 *   three members this range reads, the MTX34 at +0x1C).
 * RESIDUALS. fn_800D77B0: two registers swap - retail keeps `pRec->mFlags` in r30 and `mtxId`/the returned id in
 *   r31, ours the reverse (tried: declaration order, `void*` vs typed `pRec`, a `prev` local, `u32` vs `s32`
 *   parameters, an early `id`, reusing `mtxId` for the return id).
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_80501390`,
 *     `MTX34Trans__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_80501390`.
 * SHAPES. File-scope `#pragma peephole off`: the five flag tests keep retail's `rlwinm` + `cmpwi`.
 */

#include "types.h"
#include "nw4r/math.h"

/* The target's flag tests keep the non-record `rlwinm` + `cmpwi` (the committed cflags fuse them into
 * `rlwinm.` + `bne`); the shutdown peephole is what fuses, so this unit was built peephole-off. */
#pragma peephole off

/* The `g3d_calcworld.cpp` owner's declarations of the matrix-id helpers this unit calls. */
#include "g3d/g3d_calcworld.h"
/* The band header's declarations of the matrix copy/concat helpers (mtx34_copy_ps/mtx34_concat, owner
 * `g3d/g3d_calcview.cpp`). */
#include "g3d/g3d_calcview.h" /* fn_800710BC/mtx34_copy_ps (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

/* The record layout this range reads.  `g3d_calcworld.cpp` carries the full `NodeMtxRec`; this is the
 * three-member view the transform needs, under its own name. size: 0x4C */
struct G3DNodeMtxRec {
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ VEC3 mScale;
    /* +0x10 */ u32 mPad10[3];
    /* +0x1C */ MTX34 mMtx;
}; /* size: 0x4C */

/* fn_80501390, owner `nw4r/fn_805012C4.cpp`, declared here. */
extern "C" void fn_80501390(MTX34* out, const MTX34* a, const VEC3* v);

extern "C" s32 fn_800D79A0(u32 value);

extern "C" s32 fn_800D77B0(MTX34* pDstMtx, VEC3* pDstScale, const MTX34* pSrcMtx,
                           const VEC3* pSrcScale, s32 mtxId, G3DNodeMtxRec* pRec) {
    u32 flags = pRec->mFlags;

    if ((flags & 2) != 0 || (flags & 4) != 0) {
        mtx34_copy_ps(pDstMtx, pSrcMtx);
    } else if ((flags & 0x20) != 0) {
        if (fn_800D79A0(mtxId) != 0) {
            VEC3 v;
            setVec3(&v, pRec->mMtx.m[0][3], pRec->mMtx.m[1][3], pRec->mMtx.m[2][3]);
            fn_80501390(pDstMtx, pSrcMtx, &v);
        } else {
            VEC3 v;
            setVec3(&v, pSrcScale->x * pRec->mMtx.m[0][3], pSrcScale->y * pRec->mMtx.m[1][3],
                        pSrcScale->z * pRec->mMtx.m[2][3]);
            fn_80501390(pDstMtx, pSrcMtx, &v);
        }
    } else if (fn_800D79A0(mtxId) != 0) {
        mtx34_concat(pDstMtx, pSrcMtx, &pRec->mMtx);
    } else {
        MTX34 mtx;
        MTX34_ctor(&mtx);
        mtx34_copy_ps(&mtx, &pRec->mMtx);
        mtx.m[0][3] *= pSrcScale->x;
        mtx.m[1][3] *= pSrcScale->y;
        mtx.m[2][3] *= pSrcScale->z;
        mtx34_concat(pDstMtx, pSrcMtx, &mtx);
    }

    u32 id;
    if ((flags & 8) != 0) {
        id = fn_800737C4(mtxId);
        copyVec3(pDstScale, pSrcScale);
    } else {
        id = fn_800737B4(mtxId);
        pDstScale->x = pSrcScale->x * pRec->mScale.x;
        pDstScale->y = pSrcScale->y * pRec->mScale.y;
        pDstScale->z = pSrcScale->z * pRec->mScale.z;
    }

    if ((flags & 0x10) != 0) {
        return (s32) fn_800737BC(id);
    }
    return (s32) fn_800737AC(id);
}

/* `(value & 0x80000000) != 0` - the matrix slot's "no transform" sign bit. */
extern "C" s32 fn_800D79A0(u32 value) {
    return (value & 0x80000000u) != 0;
}
