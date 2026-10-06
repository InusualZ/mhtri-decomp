/*
 * g3d/g3d_xsi.cpp - nw4r g3d's texture-SRT matrix builder for the XSI convention.
 * RANGE. .text 0x800D6C00-0x800D77B0 (15 functions); extab, extabindex, .rodata 0x8056F830-0x8056F868 (the two
 *   seven-entry function tables), .data 0x80595790-0x80595840 ("g3d_xsi.cpp" and the three assert strings
 *   g3d_calc_tex_mtx_xsi reads), .sdata2 0x807963C8-0x807963D0 (0.0f, 1.0f).  The left edge is the end of
 *   fn_800D5F9C in `nw_resource.cpp`'s band; the right edge is `g3d/fn_800D77B0.cpp` (tudiscover at 0x800D74E8:
 *   `.sdata2` 0x807963CC -> 0x807963D0).
 * NAMES. The TU name is the `__FILE__` string.  g3d_calc_tex_mtx_xsi is a GUESS (nw4r's CalcTexMtx for the XSI
 *   convention: the `TexSrt::FLAG_ANM_EXISTS` assert and the three-bit scale/rotate/translate selector);
 *   xsi_set_s is a GUESS, xsi_set_r is a GUESS, xsi_set_t is a GUESS, xsi_set_sr is a GUESS, xsi_set_rt is a
 *   GUESS, xsi_set_st is a GUESS, xsi_set_srt is a GUESS, xsi_mul_s is a GUESS, xsi_mul_r is a GUESS, xsi_mul_t is
 *   a GUESS, xsi_mul_sr is a GUESS, xsi_mul_rt is a GUESS, xsi_mul_st is a GUESS, xsi_mul_srt is a GUESS (the
 *   table entries: which of scale, rotation and translation each one applies, writing or multiplying).
 * RESIDUALS. xsi_mul_sr, xsi_mul_srt: register choice only (retail keeps the four scale-rotation products in
 *   f0-f3, ours in f4-f7).  flipcheck: `.rodata`, `.data` and `.sdata2` are claimed and emitted by the source
 *   (the tables, the asserts' strings and the 0.0f/1.0f pool) and byte-identical to the target's (datagap).
 * SHAPES. File-scope `#pragma fp_contract off` (retail keeps every `fmuls` + `fadds`/`fsubs`), `#pragma pool_data off`
 *   (each assert string gets its own `lis`/`addi`) and `#pragma peephole off` (the kept `clrlwi` + `cmpwi` of the
 *   flag test).  The two rows of a multiply are computed into temporaries before either is stored, and the
 *   translated row 3 reads `m[1][3] + Tv` in place.  The tables are function-local `static const` arrays.
 */

#include "types.h"
#include "nw4r/math.h"
#include "g3d/g3d_resmat.h"    /* nw4r::g3d::TexSrt (rule 2) */
#include "g3d/fn_80075DCC.h"   /* sin_cos_deg (rule 2) */
#include "g3d/g3d_xsi.h"

#pragma fp_contract off
#pragma pool_data off
#pragma peephole off

namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The nw4r resource pointer assert: `ptr` must fall in one of the seven mapped Wii memory ranges. */
#define G3D_XSI_POINTER_ASSERT(ptr, line, msg)                                                 \
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
            nw4r::db::Panic("g3d_xsi.cpp", line, msg, (ptr));                                    \
    }

typedef nw4r::math::MTX34 Mtx34Xsi;
typedef nw4r::g3d::TexSrt TexSrtXsi;

/* A table entry: fills or multiplies the first two rows of `pMtx` from `srt`. */
typedef void (*XsiTexMtxFunc)(Mtx34Xsi* pMtx, const TexSrtXsi& srt);

extern "C" {

/* 0x800D6C00 (0x38): writes the scale-only matrix. */
static void xsi_set_s(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    pMtx->m[0][0] = srt.Su;
    pMtx->m[0][1] = 0.0f;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = 0.0f;
    pMtx->m[1][0] = 0.0f;
    pMtx->m[1][1] = srt.Sv;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = 1.0f - srt.Sv;
}

/* 0x800D6C38 (0x84): writes the rotation-only matrix. */
static void xsi_set_r(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    pMtx->m[0][0] = c;
    pMtx->m[0][1] = -s;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = s;
    pMtx->m[1][0] = s;
    pMtx->m[1][1] = c;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = 1.0f - c;
}

/* 0x800D6CBC (0x38): writes the translation-only matrix. */
static void xsi_set_t(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    pMtx->m[0][0] = 1.0f;
    pMtx->m[0][1] = 0.0f;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = -srt.Tu;
    pMtx->m[1][0] = 0.0f;
    pMtx->m[1][1] = 1.0f;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = srt.Tv;
}

/* 0x800D6CF4 (0x98): writes the scale-and-rotation matrix. */
static void xsi_set_sr(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    f32 suC = srt.Su * c;
    f32 suS = srt.Su * s;
    f32 svC = srt.Sv * c;
    f32 svS = srt.Sv * s;
    pMtx->m[0][0] = suC;
    pMtx->m[0][1] = -suS;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = suS;
    pMtx->m[1][0] = svS;
    pMtx->m[1][1] = svC;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = 1.0f + -svC;
}

/* 0x800D6D8C (0xC0): writes the rotation-and-translation matrix. */
static void xsi_set_rt(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    pMtx->m[0][0] = c;
    pMtx->m[0][1] = -s;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = (s - c * srt.Tu) - s * srt.Tv;
    pMtx->m[1][0] = s;
    pMtx->m[1][1] = c;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = 1.0f + ((-c - s * srt.Tu) + c * srt.Tv);
}

/* 0x800D6E4C (0x50): writes the scale-and-translation matrix. */
static void xsi_set_st(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    pMtx->m[0][0] = srt.Su;
    pMtx->m[0][1] = 0.0f;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = -srt.Su * srt.Tu;
    pMtx->m[1][0] = 0.0f;
    pMtx->m[1][1] = srt.Sv;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = 1.0f + srt.Sv * (srt.Tv - 1.0f);
}

/* 0x800D6E9C (0xC0): writes the scale, rotation and translation matrix. */
static void xsi_set_srt(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    f32 suC = srt.Su * c;
    f32 suS = srt.Su * s;
    f32 svC = srt.Sv * c;
    f32 svS = srt.Sv * s;
    pMtx->m[0][0] = suC;
    pMtx->m[0][1] = -suS;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = (suS - suC * srt.Tu) - suS * srt.Tv;
    pMtx->m[1][0] = svS;
    pMtx->m[1][1] = svC;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = 1.0f + ((-svC - svS * srt.Tu) + svC * srt.Tv);
}

/* 0x800D6F5C (0x78): scales the first two rows. */
static void xsi_mul_s(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    pMtx->m[0][0] *= srt.Su;
    pMtx->m[0][1] *= srt.Su;
    pMtx->m[0][2] *= srt.Su;
    pMtx->m[0][3] *= srt.Su;
    pMtx->m[1][0] *= srt.Sv;
    pMtx->m[1][1] *= srt.Sv;
    pMtx->m[1][2] *= srt.Sv;
    pMtx->m[1][3] = 1.0f + srt.Sv * (pMtx->m[1][3] - 1.0f);
}

/* 0x800D6FD4 (0x10C): rotates the first two rows. */
static void xsi_mul_r(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    f32 x0 = c * pMtx->m[0][0] - s * pMtx->m[1][0];
    f32 y0 = s * pMtx->m[0][0] + c * pMtx->m[1][0];
    pMtx->m[0][0] = x0;
    pMtx->m[1][0] = y0;
    f32 x1 = c * pMtx->m[0][1] - s * pMtx->m[1][1];
    f32 y1 = s * pMtx->m[0][1] + c * pMtx->m[1][1];
    pMtx->m[0][1] = x1;
    pMtx->m[1][1] = y1;
    f32 x2 = c * pMtx->m[0][2] - s * pMtx->m[1][2];
    f32 y2 = s * pMtx->m[0][2] + c * pMtx->m[1][2];
    pMtx->m[0][2] = x2;
    pMtx->m[1][2] = y2;
    f32 x3 = s + (c * pMtx->m[0][3] - s * pMtx->m[1][3]);
    f32 y3 = 1.0f + ((s * pMtx->m[0][3] + c * pMtx->m[1][3]) - c);
    pMtx->m[0][3] = x3;
    pMtx->m[1][3] = y3;
}

/* 0x800D70E0 (0x24): translates the first two rows. */
static void xsi_mul_t(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    pMtx->m[0][3] -= srt.Tu;
    pMtx->m[1][3] += srt.Tv;
}

/* 0x800D7104 (0x114): scales and rotates the first two rows. */
static void xsi_mul_sr(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    f32 suC = srt.Su * c;
    f32 suS = srt.Su * s;
    f32 svC = srt.Sv * c;
    f32 svS = srt.Sv * s;
    f32 x0 = suC * pMtx->m[0][0] - suS * pMtx->m[1][0];
    f32 y0 = svS * pMtx->m[0][0] + svC * pMtx->m[1][0];
    pMtx->m[0][0] = x0;
    pMtx->m[1][0] = y0;
    f32 x1 = suC * pMtx->m[0][1] - suS * pMtx->m[1][1];
    f32 y1 = svS * pMtx->m[0][1] + svC * pMtx->m[1][1];
    pMtx->m[0][1] = x1;
    pMtx->m[1][1] = y1;
    f32 x2 = suC * pMtx->m[0][2] - suS * pMtx->m[1][2];
    f32 y2 = svS * pMtx->m[0][2] + svC * pMtx->m[1][2];
    pMtx->m[0][2] = x2;
    pMtx->m[1][2] = y2;
    f32 x3 = suS + (suC * pMtx->m[0][3] - suS * pMtx->m[1][3]);
    f32 y3 = 1.0f + ((svS * pMtx->m[0][3] + svC * pMtx->m[1][3]) - svC);
    pMtx->m[0][3] = x3;
    pMtx->m[1][3] = y3;
}

/* 0x800D7218 (0x124): rotates and translates the first two rows. */
static void xsi_mul_rt(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    f32 x0 = c * pMtx->m[0][0] - s * pMtx->m[1][0];
    f32 y0 = s * pMtx->m[0][0] + c * pMtx->m[1][0];
    pMtx->m[0][0] = x0;
    pMtx->m[1][0] = y0;
    f32 x1 = c * pMtx->m[0][1] - s * pMtx->m[1][1];
    f32 y1 = s * pMtx->m[0][1] + c * pMtx->m[1][1];
    pMtx->m[0][1] = x1;
    pMtx->m[1][1] = y1;
    f32 x2 = c * pMtx->m[0][2] - s * pMtx->m[1][2];
    f32 y2 = s * pMtx->m[0][2] + c * pMtx->m[1][2];
    pMtx->m[0][2] = x2;
    pMtx->m[1][2] = y2;
    f32 a = pMtx->m[0][3] - srt.Tu;
    f32 x3 = s + (c * a - s * (pMtx->m[1][3] + srt.Tv));
    f32 y3 = 1.0f + ((s * a + c * (pMtx->m[1][3] + srt.Tv)) - c);
    pMtx->m[0][3] = x3;
    pMtx->m[1][3] = y3;
}

/* 0x800D733C (0x88): scales and translates the first two rows. */
static void xsi_mul_st(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    pMtx->m[0][0] *= srt.Su;
    pMtx->m[0][1] *= srt.Su;
    pMtx->m[0][2] *= srt.Su;
    pMtx->m[0][3] = srt.Su * (pMtx->m[0][3] - srt.Tu);
    pMtx->m[1][0] *= srt.Sv;
    pMtx->m[1][1] *= srt.Sv;
    pMtx->m[1][2] *= srt.Sv;
    pMtx->m[1][3] = 1.0f + srt.Sv * ((pMtx->m[1][3] + srt.Tv) - 1.0f);
}

/* 0x800D73C4 (0x124): scales, rotates and translates the first two rows. */
static void xsi_mul_srt(Mtx34Xsi* pMtx, const TexSrtXsi& srt) {
    f32 s;
    f32 c;
    sin_cos_deg(&s, &c, srt.R);
    f32 suC = srt.Su * c;
    f32 suS = srt.Su * s;
    f32 svC = srt.Sv * c;
    f32 svS = srt.Sv * s;
    f32 x0 = suC * pMtx->m[0][0] - suS * pMtx->m[1][0];
    f32 y0 = svS * pMtx->m[0][0] + svC * pMtx->m[1][0];
    pMtx->m[0][0] = x0;
    pMtx->m[1][0] = y0;
    f32 x1 = suC * pMtx->m[0][1] - suS * pMtx->m[1][1];
    f32 y1 = svS * pMtx->m[0][1] + svC * pMtx->m[1][1];
    pMtx->m[0][1] = x1;
    pMtx->m[1][1] = y1;
    f32 x2 = suC * pMtx->m[0][2] - suS * pMtx->m[1][2];
    f32 y2 = svS * pMtx->m[0][2] + svC * pMtx->m[1][2];
    pMtx->m[0][2] = x2;
    pMtx->m[1][2] = y2;
    f32 a = pMtx->m[0][3] - srt.Tu;
    f32 x3 = suS + (suC * a - suS * (pMtx->m[1][3] + srt.Tv));
    f32 y3 = 1.0f + ((svS * a + svC * (pMtx->m[1][3] + srt.Tv)) - svC);
    pMtx->m[0][3] = x3;
    pMtx->m[1][3] = y3;
}

/* 0x800D74E8 (0x2C8): builds the texture matrix of `srt` into `pMtx` (writing it when `set`, multiplying into it
 * otherwise); returns FALSE when the transform is the identity and nothing was written. */
BOOL g3d_calc_tex_mtx_xsi(Mtx34Xsi* pMtx, BOOL set, const TexSrtXsi* pSrt, u32 flag) {
    static const XsiTexMtxFunc setFuncs[7] = {
        xsi_set_srt, xsi_set_rt, xsi_set_st, xsi_set_t, xsi_set_sr, xsi_set_r, xsi_set_s,
    };
    static const XsiTexMtxFunc mulFuncs[7] = {
        xsi_mul_srt, xsi_mul_rt, xsi_mul_st, xsi_mul_t, xsi_mul_sr, xsi_mul_r, xsi_mul_s,
    };
    G3D_XSI_POINTER_ASSERT(pMtx, 0x194, "NW4R:Pointer Error\npMtx(=%p) is not valid pointer.");
    G3D_XSI_POINTER_ASSERT(pSrt, 0x195, "NW4R:Pointer Error\n& srt(=%p) is not valid pointer.");
    if ((flag & 1) == 0) {
        nw4r::db::Panic("g3d_xsi.cpp", 0x196, "NW4R:Failed assertion (flag & TexSrt::FLAG_ANM_EXISTS) != 0");
    }
    u32 idx = (flag >> 1) & 7;
    if (idx == 7) {
        return FALSE;
    }
    if (set) {
        setFuncs[idx](pMtx, *pSrt);
    } else {
        mulFuncs[idx](pMtx, *pSrt);
    }
    pMtx->m[2][0] = 0.0f;
    pMtx->m[2][1] = 0.0f;
    pMtx->m[2][2] = 1.0f;
    pMtx->m[2][3] = 0.0f;
    return TRUE;
}

} /* extern "C" */
