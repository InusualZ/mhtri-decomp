/* g3d/fn_800D77B0.cpp - the single-matrix node transform pair, .text 0x800D77B0..0x800D79B4.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: both fn_ names this file uses are bare .text entries in config/RMHE08/symbols.txt)
 *
 * Registration - which evidence class decided it.  Class 3 (what the code does, plus the naming scheme
 * of its neighbours): the range sits between the discovery proposals `800D45AC` (`g3d_xsi.cpp`) and
 * `800D79B4` (`g3d_basic.cpp`), and its own range has no `__FILE__` string and only `zz_` dump names
 * (class 1/2 give nothing).  The code settles it: `fn_800D77B0` is the per-node world-matrix builder
 * `g3d/g3d_calcworld.cpp`'s `fn_800737CC` calls, declared there as
 *   `extern "C" s32 fn_800D77B0(void* pA, void* pB, void* pC, void* pD, s32 e, void* pF);`
 * and callable as `fn_800D77B0(pDstMtx, pDstScale, pSrcMtx, pSrcScale, prev, pRec)` (`pRec` is that
 * unit's `NodeMtxRec`; its sibling `fn_800D7D24` is the multi-matrix half at 0x800D7D24, inside the
 * `g3d_basic.cpp` proposal).  So the module is `g3d`, the language is C++ (the module's units are C++
 * and the bracketing proposals cite `g3d_xsi.cpp`/`g3d_basic.cpp`), and the name stays the map's
 * `fn_800D77B0` stem (class 4 - no name evidence, and inventing one is forbidden).
 *
 * What they are.  `fn_800D77B0` folds one node's local matrix/scale into its destination slot:
 *   * `pRec->flags` bit 0x2/0x4 - copy the source matrix straight through (`fn_8007100C`);
 *   * bit 0x20 - build a translation vector (the record's matrix translation, or the source scale
 *     component-scaled by it) and concat it onto the source matrix (`fn_80501390`);
 *   * otherwise - concat the record's own matrix, scaling its translation column by the source scale
 *     when the matrix-id predicate `fn_800D79A0` is false (`fn_800710BC`, with an identity + copy);
 *   * bits 0x8/0x10 - pick the destination scale (copy the source, or component-scale by
 *     `pRec->scale`) and the matrix-id mode bits (`fn_800737B4`/`fn_800737C4`/`fn_800737AC`/
 *     `fn_800737BC`), whose result is the function's return value.
 *
 * `fn_800D79A0(mtxId)` is `(mtxId & 0x80000000) != 0` - the matrix slot's "no transform" sign bit.
 *
 * Residual (measured against the target object with `recompile.py --measure`).  `fn_800D79A0` is
 * byte-identical (100 %, 0x14 B).  `fn_800D77B0` is **97.74 %** (0x1F0 B, the target size, every
 * instruction present in the same order) - the only difference is a two-register colouring swap:
 * the target keeps `pRec->mFlags` in **r30** and `mtxId`/the returned id in **r31**, ours swaps the
 * two (`r31`/`r30`).  Declaration order, `void*` vs typed `pRec`, a `prev` local, `u32` vs `s32`
 * parameters, an early `id` declaration and reusing `mtxId` for the return id were all measured and
 * none moves it (the `mtxId`-reuse variant scores *lower*, 97.34 %).  `#pragma peephole off` is what
 * takes it from 93.51 % to 97.74 %: the target keeps the non-record `rlwinm` + `cmpwi` for all five
 * flag tests where the committed cflags fuse them into `rlwinm.`; the unit's extab/extabindex
 * fragments match byte-for-byte.
 *
 * `pRec` is this unit's private view of `g3d_calcworld.cpp`'s `NodeMtxRec` (only the three members
 * this range touches; a distinct name keeps the one-definition rule while the two units are
 * separate).  `pRec->mtx` is the record's MTX34 at +0x1C, so its translation column is
 * `m[0][3]`/`m[1][3]`/`m[2][3]` (+0x28/+0x38/+0x48) - exactly the offsets the target reads.
 */

#include "types.h"
#include "nw4r/math.h"

/* The target's flag tests keep the non-record `rlwinm` + `cmpwi` (the committed cflags fuse them into
 * `rlwinm.` + `bne`); the shutdown peephole is what fuses, so this unit was built peephole-off. */
#pragma peephole off

/* The `g3d_calcworld.cpp` owner's declarations of the matrix-id helpers this unit calls. */
#include "g3d/g3d_calcworld.h"
/* The `g3d` band's declarations for the still-unsplit matrix copy/concat helpers. */
#include "unsplit/g3d.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* The record layout this range reads.  `g3d_calcworld.cpp` carries the full `NodeMtxRec`; this is the
 * three-member view the transform needs, under its own name. size: 0x4C */
struct G3DNodeMtxRec {
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ VEC3 mScale;
    /* +0x10 */ u32 mPad10[3];
    /* +0x1C */ MTX34 mMtx;
}; /* size: 0x4C */

/* No registered owner (their address band interleaves modules): declared here for this unit only. */
extern "C" void fn_80501390(MTX34* out, const MTX34* a, const VEC3* v);

extern "C" s32 fn_800D79A0(u32 value);

extern "C" s32 fn_800D77B0(MTX34* pDstMtx, VEC3* pDstScale, const MTX34* pSrcMtx,
                           const VEC3* pSrcScale, s32 mtxId, G3DNodeMtxRec* pRec) {
    u32 flags = pRec->mFlags;

    if ((flags & 2) != 0 || (flags & 4) != 0) {
        fn_8007100C(pDstMtx, pSrcMtx);
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
        fn_800710BC(pDstMtx, pSrcMtx, &pRec->mMtx);
    } else {
        MTX34 mtx;
        MTX34_ctor(&mtx);
        fn_8007100C(&mtx, &pRec->mMtx);
        mtx.m[0][3] *= pSrcScale->x;
        mtx.m[1][3] *= pSrcScale->y;
        mtx.m[2][3] *= pSrcScale->z;
        fn_800710BC(pDstMtx, pSrcMtx, &mtx);
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
