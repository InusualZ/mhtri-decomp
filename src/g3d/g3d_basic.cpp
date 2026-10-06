/*
 * g3d/g3d_basic.cpp - nw4r g3d basic-matrix cluster: SRT matrix build and dispatch, a node-matrix scale, a handle bit.
 * RANGE. .text 0x800D79B4-0x800D7F54 (4 functions); extab, extabindex, .data 0x80595840-0x805958F8 (opens on
 *   "g3d_basic.cpp", fn_800D79B4's assert file), .sdata2 0x807963D0-0x807963D8.
 * NAMES. Map stems.
 * RESIDUALS. fn_800D7D24: one prologue pair reordered - retail copies `pNode` to r29 and loads the flag word to r30
 *   before copying `handle` to r31, ours copies it to r30 first (tried: the local inlined or reordered, `tmpMtx`/
 *   `tmpVec` scoped into their branches; the `handleArg` -> `handle` alias is kept).
 *   flipcheck: the `.text` layout is a permutation of retail's - every function keeps its size but sits at another
 *   address, so the definitions are not in address order; `.data` and `.sdata2` are claimed and not emitted.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_8050133C`,
 *     `MTX34Scale__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3`, `fn_80501390`,
 *     `MTX34Trans__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_8050133C`, `fn_80501390`.
 * SHAPES. File-scope `#pragma peephole off`: fn_800D7F40 keeps the unfolded `(x & 0x40000000) != 0`, and
 *   fn_800D7D24 and fn_800D79B4 lose rows without it.
 */
#include "types.h"
#include "nw4r/math.h"
#include "g3d/g3d_calcview.h" /* fn_800710BC/mtx34_copy_ps (rule 2) */
#include "g3d/fn_80075DCC.h" /* fn_80075DCC, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

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
u32 fn_800737AC(u32 handle);
u32 fn_800737B4(u32 handle);
u32 fn_800737BC(u32 handle);
u32 fn_800737C4(u32 handle);
void fn_8050133C(void* pOut, const void* pA, const void* pB);
void fn_80501390(void* pOut, const void* pA, const void* pB);
void PSMTXScaleApply(Mtx34* pDst, const Mtx34* pSrc, f32 xS, f32 yS, f32 zS);
void PSMTXCopy(const Mtx34* pSrc, Mtx34* pDst);
void PSMTXConcat(const Mtx34* pA, const Mtx34* pB, Mtx34* pDst);
}

/* The pooled constants and the file-name/assert strings, declared, not defined: the claimed `.data`/`.sdata2`
 * are not emitted yet. */
extern const char lbl_80595840[]; /* "g3d_basic.cpp"                                       .data */
extern const char lbl_80595850[]; /* "NW4R:Pointer Error\npMtx(=%p) is not valid pointer." */
extern const char lbl_80595884[]; /* "NW4R:Pointer Error\n& srt(=%p) is not valid pointer." */
extern const char lbl_805958B8[]; /* "NW4R:Failed assertion (flag & TexSrt::FLAG_ANM_..."  */
extern const f32 lbl_807963D0;    /* 0.0f                                                 .sdata2 */
extern const f32 lbl_807963D4;    /* 1.0f                                                 .sdata2 */

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
            nw4r::db::Panic(lbl_80595840, line, msg, (ptr));                                   \
    }

extern "C" {

/* Test bit 0x40000000 of a resource handle. */
u32 fn_800D7F40(u32 handle) {
    u32 masked = handle & 0x40000000;
    return masked != 0;
}

/* Scale `pSrc` by the VEC3 `pScale` into the resource node's matrix and return `pSrc`. */
Mtx34* fn_800D7ED0(Mtx34* pSrc, const Vec3* pScale, void* pNodeMtx) {
    const Mtx34* src = (const Mtx34*)mtx34_get_ptr(pSrc);
    Mtx34* dst = (Mtx34*)mtx34_const_ptr(pNodeMtx);
    PSMTXScaleApply(dst, src, pScale->x, pScale->y, pScale->z);
    return pSrc;
}

/* The SRT dispatch: from the node's flag word pick a copy, a concatenation or a vector transform. */
void fn_800D7D24(Mtx34* pMtx, Vec3* pVecOut, const Mtx34* pSrcMtx, const Vec3* pScale, u32 handleArg,
                 ResNode* pNode) {
    u32 flag = pNode->flag;
    /* arg5 aliased through a local: it is the register-colouring lever for the prologue (see the file
     * header) - the flag word lands in the register retail gives it. */
    u32 handle = handleArg;
    Mtx34 tmpMtx;
    Vec3 tmpVec;

    if ((flag & 0x2) || (flag & 0x4)) {
        if (fn_800D7F40(handle)) {
            mtx34_copy_ps(pMtx, pSrcMtx);
        } else {
            fn_8050133C(pMtx, pSrcMtx, pScale);
        }
    } else if (flag & 0x20) {
        if (fn_800D7F40(handle)) {
            setVec3(&tmpVec, pNode->mtx.m[0][3], pNode->mtx.m[1][3], pNode->mtx.m[2][3]);
            fn_80501390(pMtx, pSrcMtx, &tmpVec);
        } else {
            MTX34_ctor(&tmpMtx);
            fn_800D7ED0(&tmpMtx, pScale, &pNode->mtx);
            mtx34_concat(pMtx, pSrcMtx, &tmpMtx);
        }
    } else {
        if (fn_800D7F40(handle)) {
            mtx34_concat(pMtx, pSrcMtx, &pNode->mtx);
        } else {
            fn_8050133C(pMtx, pSrcMtx, pScale);
            mtx34_concat(pMtx, pMtx, &pNode->mtx);
        }
    }

    if (flag & 0x8) {
        handle = fn_800737C4(handle);
        pVecOut->z = lbl_807963D4;
        pVecOut->y = lbl_807963D4;
        pVecOut->x = lbl_807963D4;
    } else {
        handle = fn_800737B4(handle);
        copyVec3(pVecOut, &pNode->pos);
    }

    if (flag & 0x10) {
        fn_800737BC(handle);
    } else {
        fn_800737AC(handle);
    }
}

/* Build the SRT matrix into `pMtx` (or concatenate it with `pMtx`), and report whether the flag's mode
 * is legal. */
int fn_800D79B4(Mtx34* pMtx, int direct, const Srt* pSrt, u32 flag) {
    f32 sin_, cos_;
    Mtx34 mtx;

    NW4R_POINTER_ASSERT(pMtx, 36, lbl_80595850);
    NW4R_POINTER_ASSERT(pSrt, 37, lbl_80595884);
    if (!(flag & 0x1)) {
        nw4r::db::Panic(lbl_80595840, 38, lbl_805958B8);
    }

    if (((flag >> 1) & 0x7) == 0x7) {
        return 0;
    }

    sin_cos_deg(&sin_, &cos_, pSrt->rotate);

    if (direct != 0) {
        pMtx->m[0][0] = pSrt->scaleX * cos_;
        pMtx->m[0][1] = -pSrt->scaleY * sin_;
        pMtx->m[0][2] = lbl_807963D0;
        pMtx->m[0][3] = pSrt->transX;
        pMtx->m[1][0] = pSrt->scaleX * sin_;
        pMtx->m[1][1] = pSrt->scaleY * cos_;
        pMtx->m[1][2] = lbl_807963D0;
        pMtx->m[1][3] = pSrt->transY;
        pMtx->m[2][0] = lbl_807963D0;
        pMtx->m[2][1] = lbl_807963D0;
        pMtx->m[2][2] = lbl_807963D4;
        pMtx->m[2][3] = lbl_807963D0;
    } else {
        MTX34_ctor(&mtx);
        mtx.m[0][0] = pSrt->scaleX * cos_;
        mtx.m[0][1] = -pSrt->scaleY * sin_;
        mtx.m[0][2] = lbl_807963D0;
        mtx.m[0][3] = pSrt->transX;
        mtx.m[1][0] = pSrt->scaleX * sin_;
        mtx.m[1][1] = pSrt->scaleY * cos_;
        mtx.m[1][2] = lbl_807963D0;
        mtx.m[1][3] = pSrt->transY;
        mtx.m[2][0] = lbl_807963D0;
        mtx.m[2][1] = lbl_807963D0;
        mtx.m[2][2] = lbl_807963D4;
        mtx.m[2][3] = lbl_807963D0;
        mtx34_concat(pMtx, &mtx, pMtx);
    }

    return 1;
}

} /* extern "C" */
