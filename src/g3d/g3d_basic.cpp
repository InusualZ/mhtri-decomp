/*
 * g3d/g3d_basic.cpp - nw4r g3d basic-matrix cluster: SRT matrix build and dispatch, a node-matrix scale, a handle bit.
 * RANGE. .text 0x800D79B4-0x800D7F54 (4 functions); extab, extabindex, .data 0x80595840-0x805958F8 (opens on
 *   "g3d_basic.cpp", fn_800D79B4's assert file), .sdata2 0x807963D0-0x807963D8.
 * NAMES. Map stems.
 * RESIDUALS. fn_800D7D24: retail copies `handleArg` to r31 after loading the flag word and its three bit tests pass
 *   r7 itself; ours copies it first and the tests pass r31 (the registers match: `handle` declared first, assigned
 *   after the flag word, the tests on the argument; tried: `tmpMtx`/`tmpVec` scoped or first, `handle` initialised
 *   in its declaration).
 *   flipcheck: `.data` and `.sdata2` byte-identical.
 * SHAPES. File-scope `#pragma peephole off`: fn_800D7F40 keeps the unfolded `(x & 0x40000000) != 0`, and
 *   fn_800D7D24 and fn_800D79B4 lose rows without it.  `#pragma pool_data off` gives every string its own
 *   `lis`/`addi`.  The definitions are in retail's address order, which also orders the `.data` strings and the
 *   0.0f/1.0f `.sdata2` pool (first use).
 */
#include "types.h"
#include "nw4r/math.h"
#include "g3d/g3d_calcview.h" /* fn_800710BC/mtx34_copy_ps (rule 2) */
#include "g3d/g3d_camera.h" /* sin_cos_deg (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "nw4r/math_types.h" /* nw4r::math::MTX34Scale / MTX34Trans, owner nw4r/math_types.cpp (rule 2) */

#pragma pool_data off

/* The retail object keeps the un-folded `(x & mask) != 0` form in fn_800D7F40 (`rlwinm` + the
 * neg/or/srwi tests) and the un-fused compares elsewhere; the file-scope peephole pass folds both.
 * Measured with `-opt nopeephole`. */
#pragma peephole off

namespace nw4r {
namespace db {
/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling. */
void Panic(const char* pFile, int line, const char* pFmt, ...);
}  // namespace db
}  // namespace nw4r

extern "C" {
/* The `g3d` node/resource helpers this unit calls (plain map stems). */
void* mtx34_get_ptr(void* pMtx);
void* mtx34_const_ptr(void* pMtx);
u32 world_mtx_attr_not_scale_uniform(u32 handle);
u32 world_mtx_attr_not_scale_one(u32 handle);
u32 world_mtx_attr_scale_uniform(u32 handle);
u32 world_mtx_attr_scale_one(u32 handle);
void PSMTXScaleApply(Mtx34* pDst, const Mtx34* pSrc, f32 xS, f32 yS, f32 zS);
void PSMTXCopy(const Mtx34* pSrc, Mtx34* pDst);
void PSMTXConcat(const Mtx34* pA, const Mtx34* pB, Mtx34* pDst);
}

/* The `Srt` record fn_800D79B4 reads: a 2-D scale, a rotation and a 2-D translation.
 * size: 0x14 (only these five fields are reached; a lower bound) */
struct Srt {
    /* +0x00 */ f32 scaleX;
    /* +0x04 */ f32 scaleY;
    /* +0x08 */ f32 rotate;
    /* +0x0C */ f32 transX;
    /* +0x10 */ f32 transY;
};

/* The g3d resource node fn_800D7D24 reads: the flag word, a position, and the node's 3x4 matrix whose
 * translation column is read at +0x28/+0x38/+0x48.
 * size: 0x4C (only the fields below are reached; a lower bound) */
struct ResNode {
    /* +0x00 */ u32 flag;
    /* +0x04 */ Vec3 pos;
    /* +0x10 */ u8 pad_0x10[0x0C];
    /* +0x1C */ Mtx34 mtx;
};

/* The nw4r pointer assert: `addr` must fall in one of the seven mapped ranges.  Six materialised BOOLs
 * and the two-test first `if` are the target's exact shape (the same expansion ef/ef_point.cpp needed). */
#define NW4R_POINTER_ASSERT(ptr, line, msg)                                                    \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;     \
        u32 top_ = (u32)(ptr)&0xFF000000u;                                                     \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr)&0xFF800000u) == 0x81000000u))              \
            ok6_ = FALSE;                                                                      \
        if (!ok6_ && !(((u32)(ptr)&0xF8000000u) == 0x90000000u))                               \
            ok5_ = FALSE;                                                                      \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                   \
            ok4_ = FALSE;                                                                      \
        if (!ok4_ && !(((u32)(ptr)&0xFF800000u) == 0xC1000000u))                               \
            ok3_ = FALSE;                                                                      \
        if (!ok3_ && !(((u32)(ptr)&0xF8000000u) == 0xD0000000u))                               \
            ok2_ = FALSE;                                                                      \
        if (!ok2_ && !(((u32)(ptr)&0xFFFFC000u) == 0xE0000000u))                               \
            ok1_ = FALSE;                                                                      \
        if (!ok1_)                                                                             \
            nw4r::db::Panic("g3d_basic.cpp", line, msg, (ptr));                                   \
    }

extern "C" {

/* The unit's own functions, in retail's order. */
int fn_800D79B4(Mtx34* pMtx, int direct, const Srt* pSrt, u32 flag);
Mtx34* fn_800D7ED0(Mtx34* pSrc, const Vec3* pScale, void* pNodeMtx);
u32 fn_800D7F40(u32 handle);

/* Build the SRT matrix into `pMtx` (or concatenate it with `pMtx`), and report whether the flag's mode
 * is legal. */
int fn_800D79B4(Mtx34* pMtx, int direct, const Srt* pSrt, u32 flag) {
    f32 sin_, cos_;
    Mtx34 mtx;

    NW4R_POINTER_ASSERT(pMtx, 36, "NW4R:Pointer Error\npMtx(=%p) is not valid pointer.");
    NW4R_POINTER_ASSERT(pSrt, 37, "NW4R:Pointer Error\n& srt(=%p) is not valid pointer.");
    if (!(flag & 0x1)) {
        nw4r::db::Panic("g3d_basic.cpp", 38, "NW4R:Failed assertion (flag & TexSrt::FLAG_ANM_EXISTS) != 0");
    }

    if (((flag >> 1) & 0x7) == 0x7) {
        return 0;
    }

    sin_cos_deg(&sin_, &cos_, pSrt->rotate);

    if (direct != 0) {
        pMtx->m[0][0] = pSrt->scaleX * cos_;
        pMtx->m[0][1] = -pSrt->scaleY * sin_;
        pMtx->m[0][2] = 0.0f;
        pMtx->m[0][3] = pSrt->transX;
        pMtx->m[1][0] = pSrt->scaleX * sin_;
        pMtx->m[1][1] = pSrt->scaleY * cos_;
        pMtx->m[1][2] = 0.0f;
        pMtx->m[1][3] = pSrt->transY;
        pMtx->m[2][0] = 0.0f;
        pMtx->m[2][1] = 0.0f;
        pMtx->m[2][2] = 1.0f;
        pMtx->m[2][3] = 0.0f;
    } else {
        MTX34_ctor(&mtx);
        mtx.m[0][0] = pSrt->scaleX * cos_;
        mtx.m[0][1] = -pSrt->scaleY * sin_;
        mtx.m[0][2] = 0.0f;
        mtx.m[0][3] = pSrt->transX;
        mtx.m[1][0] = pSrt->scaleX * sin_;
        mtx.m[1][1] = pSrt->scaleY * cos_;
        mtx.m[1][2] = 0.0f;
        mtx.m[1][3] = pSrt->transY;
        mtx.m[2][0] = 0.0f;
        mtx.m[2][1] = 0.0f;
        mtx.m[2][2] = 1.0f;
        mtx.m[2][3] = 0.0f;
        mtx34_concat(pMtx, &mtx, pMtx);
    }

    return 1;
}

/* The SRT dispatch: from the node's flag word pick a copy, a concatenation or a vector transform. */
void fn_800D7D24(Mtx34* pMtx, Vec3* pVecOut, const Mtx34* pSrcMtx, const Vec3* pScale, u32 handleArg,
                 ResNode* pNode) {
    u32 handle;
    u32 flag = pNode->flag;
    handle = handleArg;
    Mtx34 tmpMtx;
    Vec3 tmpVec;

    if ((flag & 0x2) || (flag & 0x4)) {
        if (fn_800D7F40(handleArg)) {
            mtx34_copy_ps(pMtx, pSrcMtx);
        } else {
            nw4r::math::MTX34Scale(pMtx, pSrcMtx, pScale);
        }
    } else if (flag & 0x20) {
        if (fn_800D7F40(handleArg)) {
            setVec3(&tmpVec, pNode->mtx.m[0][3], pNode->mtx.m[1][3], pNode->mtx.m[2][3]);
            nw4r::math::MTX34Trans(pMtx, pSrcMtx, &tmpVec);
        } else {
            MTX34_ctor(&tmpMtx);
            fn_800D7ED0(&tmpMtx, pScale, &pNode->mtx);
            mtx34_concat(pMtx, pSrcMtx, &tmpMtx);
        }
    } else {
        if (fn_800D7F40(handleArg)) {
            mtx34_concat(pMtx, pSrcMtx, &pNode->mtx);
        } else {
            nw4r::math::MTX34Scale(pMtx, pSrcMtx, pScale);
            mtx34_concat(pMtx, pMtx, &pNode->mtx);
        }
    }

    if (flag & 0x8) {
        handle = world_mtx_attr_scale_one(handle);
        pVecOut->z = 1.0f;
        pVecOut->y = 1.0f;
        pVecOut->x = 1.0f;
    } else {
        handle = world_mtx_attr_not_scale_one(handle);
        copyVec3(pVecOut, &pNode->pos);
    }

    if (flag & 0x10) {
        world_mtx_attr_scale_uniform(handle);
    } else {
        world_mtx_attr_not_scale_uniform(handle);
    }
}

/* Scale `pSrc` by the VEC3 `pScale` into the resource node's matrix and return `pSrc`. */
Mtx34* fn_800D7ED0(Mtx34* pSrc, const Vec3* pScale, void* pNodeMtx) {
    const Mtx34* src = (const Mtx34*)mtx34_get_ptr(pSrc);
    Mtx34* dst = (Mtx34*)mtx34_const_ptr(pNodeMtx);
    PSMTXScaleApply(dst, src, pScale->x, pScale->y, pScale->z);
    return pSrc;
}

/* Test bit 0x40000000 of a resource handle. */
u32 fn_800D7F40(u32 handle) {
    u32 masked = handle & 0x40000000;
    return masked != 0;
}

} /* extern "C" */
