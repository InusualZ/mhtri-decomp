/* g3d/g3d_basic.cpp - the `g3d_basic.cpp` basic-matrix cluster, .text 0x800D79B4..0x800D7F54.
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every name
 * this file uses is a bare `fn_XXXXXXXX` entry in config/RMHE08/symbols.txt).
 *
 * The file name is class-1 evidence: the `.data` string `lbl_80595840` is `"g3d_basic.cpp"`, the `pFile`
 * argument of every `nw4r::db::Panic` call in fn_800D79B4 (read out of orig/RMHE08/sys/main.dol). The
 * `.cpp` suffix and the `Panic__Q24nw4r2dbFPCciPCce` callee make it C++. Module `g3d`, so
 * `src/g3d/g3d_basic.cpp`.
 *
 * The four functions build 2-D scale/rotate/translate matrices from an SRT record and multiply them into
 * a model matrix:
 *   - fn_800D7F40 is the `0x40000000` handle-bit test.
 *   - fn_800D7ED0 scales a matrix by a VEC3 (into a resource node's matrix).
 *   - fn_800D7D24 is the SRT dispatch: it picks a direct copy / concat / vector-transform path from the
 *     node's flag word (bits 0x2, 0x4, 0x8, 0x10, 0x20).
 *   - fn_800D79B4 builds the SRT matrix itself (scale/rotate/translation in the XY plane) with the
 *     `nw4r::db::Panic` pointer asserts at lines 36-38.
 *
 * Residuals and the measured per-symbol table are in the outbox.
 *
 * Measured per symbol against the split target `build/RMHE08/obj/g3d/g3d_basic.o` (official
 * `fuzzy_match_percent`): fn_800D79B4 100.0, fn_800D7ED0 100.0, fn_800D7F40 100.0, fn_800D7D24 97.336.
 *   - fn_800D7F40 needs the un-folded `(x & 0x40000000) != 0` shape, so the file carries
 *     `#pragma peephole off`; the same pragma is what the other three want (without it fn_800D7D24 is
 *     92.15 and fn_800D79B4 99.52).
 *   - fn_800D7D24 (97.336 %) is instruction-for-instruction equal to retail: 128 instructions, the same
 *     opcode sequence except one prologue pair reordered (retail copies `pNode` to r29 and loads the flag
 *     word to r30 before copying `handle`/arg5 to r31; ours copies arg5 to r30 first). Its 428 bytes are
 *     identical; only the callee-saved colouring differs. Shapes tried: arg5 aliased through a local
 *     (`handleArg` -> `handle`, the form kept - 97.336), the local inlined or reordered (97.056), and
 *     `tmpMtx`/`tmpVec` scoped into their branches (97.056).
 */
#include "types.h"
#include "nw4r/math.h"
#include "unsplit/g3d.h" /* fn_8007100C / fn_800710BC / fn_80075DCC (rule 2) */

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
/* The `g3d` node/resource helpers this unit calls; all still `fn_XXXXXXXX` in the map.  Declared here
 * because their addresses have no registered owner yet or are declared in the unsplit band. */
void fn_80041E40(Vec3* pDst, const Vec3* pSrc);
void fn_80041E8C(Vec3* pOut, f32 x, f32 y, f32 z);
void* fn_80050508(void* pMtx);
void fn_8005050C(void* pOut);
void* fn_80051570(void* pMtx);
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

/* The pooled constants and the file-name/assert strings (declared, never defined - they live in the
 * original `.data`/`.sdata2`, which this unit does not claim). */
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
    const Mtx34* src = (const Mtx34*)fn_80050508(pSrc);
    Mtx34* dst = (Mtx34*)fn_80051570(pNodeMtx);
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
            fn_8007100C(pMtx, pSrcMtx);
        } else {
            fn_8050133C(pMtx, pSrcMtx, pScale);
        }
    } else if (flag & 0x20) {
        if (fn_800D7F40(handle)) {
            fn_80041E8C(&tmpVec, pNode->mtx.m[0][3], pNode->mtx.m[1][3], pNode->mtx.m[2][3]);
            fn_80501390(pMtx, pSrcMtx, &tmpVec);
        } else {
            fn_8005050C(&tmpMtx);
            fn_800D7ED0(&tmpMtx, pScale, &pNode->mtx);
            fn_800710BC(pMtx, pSrcMtx, &tmpMtx);
        }
    } else {
        if (fn_800D7F40(handle)) {
            fn_800710BC(pMtx, pSrcMtx, &pNode->mtx);
        } else {
            fn_8050133C(pMtx, pSrcMtx, pScale);
            fn_800710BC(pMtx, pMtx, &pNode->mtx);
        }
    }

    if (flag & 0x8) {
        handle = fn_800737C4(handle);
        pVecOut->z = lbl_807963D4;
        pVecOut->y = lbl_807963D4;
        pVecOut->x = lbl_807963D4;
    } else {
        handle = fn_800737B4(handle);
        fn_80041E40(pVecOut, &pNode->pos);
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

    fn_80075DCC(&sin_, &cos_, pSrt->rotate);

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
        fn_8005050C(&mtx);
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
        fn_800710BC(pMtx, &mtx, pMtx);
    }

    return 1;
}

} /* extern "C" */
