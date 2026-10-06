/* g3d/g3d_gpu - the nw4r g3d fifo current-matrix-index and 3x3 texture-matrix display-list writers.
 * RANGE. .text 0x8009B140-0x8009B374 (2 functions); extab, extabindex, .data 0x80591900-0x80591948 (the file name
 *   and the assert message), .sdata2 0x80795F6C-0x80795F70 (0.0f).
 * FLAGS. cflags_g3d (docs/g3d.md).
 * NAMES. GDSetCurrentMtx and GDLoadTexMtxImm3x3 are GUESSES from the nw4r g3d fifo API (XF 0x1018 matrix-index
 *   packing; a 3x3 expanded to 3x4 and loaded with GXLoadTexMtxImm); `Array8` is the assert message's name, `Mat33`
 *   a GUESS.
 * RESIDUALS. none.
 * SHAPES. `GXLoadTexMtxImm` is declared `extern "C"` (the map's plain name); the assert message is retail's bytes,
 *   `\n` then a plain `t`; the strings are literals so the object emits `.data` in retail's order.
 */

#include "types.h"
#include "nw4r/math.h"      /* nw4r::math::MTX34 */
#include "gx/fn_8009AA78.h" /* fn_8009AB1C / fn_8009AC44 - owner gx/fn_8009AA78.c (rule 2) */
#include "fn_8004CAD8.h"    /* MTX34_ctor / fn_80050508 - owner src/fn_8004CAD8.cpp (rule 2) */
#include "g3d/g3d_gpu.h"

/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map's name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling
 * (rule 9).  nw4r::db is unsplit, so the declaration lives with its consumers. */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* The SDK's `GXLoadTexMtxImm`, C linkage to match the map's plain name (C++ would emit `GXLoadTexMtxImm__FPCvUlUl`);
 * the third argument is a `GXTexMtxType` (`GX_MTX3x4 == 0`). */
extern "C" void GXLoadTexMtxImm(const void* pMtx, u32 id, u32 type);

/* The nw4r resource pointer assert: `ptr` must fall in one of the seven mapped Wii memory ranges.  The six
 * materialised BOOLs and the two-test first `if` (`top_` caches the 0xFF000000 test the 0xC0000000 test reuses)
 * are retail's shape; `line` is the original file's line (`li r4, 0xff`). */
#define G3D_GPU_POINTER_ASSERT(ptr, line, msg)                                                 \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic("g3d_gpu.cpp", line, msg, (ptr));                                    \
    }

extern "C" {

/* Emits the texgen record's eight settings as the two packed command words of the XF `0x1018` state,
 * plus their fixed header.  Caller: fn_80087CCC (0x80087D78), which packs the record it just built. */
void GDSetCurrentMtx(Array8* self) {
    G3D_GPU_POINTER_ASSERT(self, 255, "NW4R:Pointer Error\ntArray8(=%p) is not valid pointer.");

    u32 hi = (self->hi_bits_6 << 6) | (self->hi_bits_12 << 12) | (self->hi_bits_18 << 18)
           | (self->hi_bits_24 << 24);
    u32 lo = self->lo_bits_0 | (self->lo_bits_6 << 6) | (self->lo_bits_12 << 12)
           | (self->lo_bits_18 << 18);

    fn_8009AC44(0x1018, 2);
    fn_8009AB1C(hi);
    fn_8009AB1C(lo);
}

/* Expands the stored 3x3 rotation `pSrc` into a 3x4 texture matrix (fourth column the pooled 0.0f) and loads it
 * as texgen matrix `id`; fn_80087CCC (0x80087D24) calls it once per setting that wants a matrix. */
void GDLoadTexMtxImm3x3(const Mat33* pSrc, u32 id) {
    nw4r::math::MTX34 mtx;

    MTX34_ctor(&mtx);
    mtx.m[0][0] = pSrc->m[0][0];
    mtx.m[0][1] = pSrc->m[0][1];
    mtx.m[0][2] = pSrc->m[0][2];
    mtx.m[0][3] = 0.0f;
    mtx.m[1][0] = pSrc->m[1][0];
    mtx.m[1][1] = pSrc->m[1][1];
    mtx.m[1][2] = pSrc->m[1][2];
    mtx.m[1][3] = 0.0f;
    mtx.m[2][0] = pSrc->m[2][0];
    mtx.m[2][1] = pSrc->m[2][1];
    mtx.m[2][2] = pSrc->m[2][2];
    mtx.m[2][3] = 0.0f;

    GXLoadTexMtxImm(mtx34_get_ptr(&mtx), id, 0 /* GX_MTX3x4 */);
}

} /* extern "C" */
