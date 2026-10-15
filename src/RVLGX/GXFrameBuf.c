/*
 * RVLGX/GXFrameBuf.c - the SDK GX frame-buffer copy (display copy source/dest/filter/gamma, copy clear, bounding box).
 *
 * RANGE. `.text` 0x804B6C00-0x804B77E0 (15 functions, `GXAdjustForOverscan` .. `GXClearBoundingBox`); `.data`
 *   0x8061A9C0-0x8061AAF0 (the five render-mode objects); `.sdata2` 0x8079D118-0x8079D128 (256.0f and the u32-to-float
 *   conversion constant, read by `GXGetYScaleFactor` and `GXSetDispCopyYScale` only).
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p).
 * NAMES. SDK names throughout. The render modes are read by `__GXInitGX` (`RVLGX/GXInit.c`), `main.cpp` and
 *   `g3d/g3d_init.cpp`. GUESS: `GXNtsc480ProgSoft` for the progressive NTSC mode at 0x8061A9FC (its vfilter is the
 *   soft {8,8,10,12,10,8,8}, where the SDK's `GXNtsc480Prog` carries {0,0,21,22,21,0,0}).
 * RESIDUALS. none in `.text`/`.sdata2`; the `.text` claim ends with 8 bytes of link padding. `.data` is 0x12C bytes where
 *   `splits.txt` claims 0x130: the map sizes `GXEurgb60Hz480IntDf` 0x40 (a 0x3C object plus 4 bytes of alignment fill),
 *   so that one row does not pair at full size until the map row is 0x3C and the `.data` claim ends at 0x8061AAEC. The
 *   pool labels are compiler-local (`@NNN`) where the map names them `lbl_8079D118`/`lbl_8079D120`: same offsets and
 *   bytes.
 * SHAPES. The BP words are built with `GX_SET_REG` (`__rlwimi`) on scalars. `GXSetDispCopyDst` inserts into
 *   `cpDispStride` in place (a scalar built first swaps r3/r4). `GXSetTexCopyDst` reads `__GXData` at every access (a
 *   `gx` local is kept in r31 instead of the target's reload and `&cpTex` in r29). `GXAdjustForOverscan` reads
 *   `viTVmode & 3` once into a local and divides as `u32` (`divwu`). `__GXGetNumXfbLines` is `static inline`: a plain
 *   `static` is inlined at both callers and also emitted out of line (0x54 B the target does not have).
 */

#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXTexture.h"

GXRenderModeObj GXNtsc480IntDf = {
    0, 640, 480, 480, 40, 0, 640, 480, 0, 1, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {8, 8, 10, 12, 10, 8, 8},
};

GXRenderModeObj GXNtsc480ProgSoft = {
    2, 640, 480, 480, 40, 0, 640, 480, 0, 0, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {8, 8, 10, 12, 10, 8, 8},
};

GXRenderModeObj GXMpal480IntDf = {
    8, 640, 480, 480, 40, 0, 640, 480, 0, 1, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {8, 8, 10, 12, 10, 8, 8},
};

GXRenderModeObj GXPal528IntDf = {
    4, 640, 528, 528, 40, 23, 640, 528, 0, 1, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {8, 8, 10, 12, 10, 8, 8},
};

GXRenderModeObj GXEurgb60Hz480IntDf = {
    20, 640, 480, 480, 40, 0, 640, 480, 0, 1, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {8, 8, 10, 12, 10, 8, 8},
};

/* Copies `rmin` to `rmout` and shrinks the copy by `hor` pixels and `ver` lines of overscan on each side. */
void GXAdjustForOverscan(const GXRenderModeObj* rmin, GXRenderModeObj* rmout, u16 hor, u16 ver)
{
    u16 hor2 = (u16)(hor * 2);
    u16 ver2 = (u16)(ver * 2);
    u32 verf;
    u32 tvMode;

    if (rmin != rmout) {
        *rmout = *rmin;
    }
    tvMode = rmin->viTVmode & 3;
    rmout->fbWidth = rmin->fbWidth - hor2;
    verf = (u32)(ver2 * rmin->efbHeight) / rmin->xfbHeight;
    rmout->efbHeight = rmin->efbHeight - verf;
    if (rmin->xfbMode == 0 && tvMode == 0) {
        rmout->xfbHeight = rmin->xfbHeight - ver2 / 2;
    } else {
        rmout->xfbHeight = rmin->xfbHeight - ver2;
    }
    rmout->viWidth = rmin->viWidth - hor2;
    if (tvMode == 1) {
        rmout->viHeight = rmin->viHeight - ver2 * 2;
    } else {
        rmout->viHeight = rmin->viHeight - ver2;
    }
    rmout->viXOrigin = rmin->viXOrigin + hor;
    rmout->viYOrigin = rmin->viYOrigin + ver;
}

/* Sets the source rectangle of the display copy. */
void GXSetDispCopySrc(u16 left, u16 top, u16 wd, u16 ht)
{
    GXData* gx = __GXData;
    u32 src;
    u32 size;

    src = 0;
    GX_SET_REG(src, left, 22, 31);
    GX_SET_REG(src, top, 12, 21);
    GX_SET_REG(src, 0x49, 0, 7);
    gx->cpDispSrc.word = src;
    size = 0;
    GX_SET_REG(size, wd - 1, 22, 31);
    GX_SET_REG(size, ht - 1, 12, 21);
    GX_SET_REG(size, 0x4A, 0, 7);
    gx->cpDispSize.word = size;
}

/* Sets the source rectangle of the texture copy. */
void GXSetTexCopySrc(u16 left, u16 top, u16 wd, u16 ht)
{
    GXData* gx = __GXData;
    u32 src;
    u32 size;

    src = 0;
    GX_SET_REG(src, left, 22, 31);
    GX_SET_REG(src, top, 12, 21);
    GX_SET_REG(src, 0x49, 0, 7);
    gx->cpTexSrc.word = src;
    size = 0;
    GX_SET_REG(size, wd - 1, 22, 31);
    GX_SET_REG(size, ht - 1, 12, 21);
    GX_SET_REG(size, 0x4A, 0, 7);
    gx->cpTexSize.word = size;
}

/* Sets the stride of the display copy's destination from its width. */
void GXSetDispCopyDst(u16 wd, u16 ht)
{
    GXData* gx = __GXData;
    u16 stride = (u16)(wd * 2);

    gx->cpDispStride.word = 0;
    GX_SET_REG(gx->cpDispStride.word, stride >> 5, 22, 31);
    GX_SET_REG(gx->cpDispStride.word, 0x4D, 0, 7);
}

/* Sets the size, format and mipmap filter of the texture copy's destination. */
void GXSetTexCopyDst(u16 wd, u16 ht, s32 fmt, u8 mipmap)
{
    u32 rowTiles;
    u32 colTiles;
    u32 cmpTiles;
    u32 peTexFmt;

    peTexFmt = fmt & 0xF;
    __GXData->cpTexZ = 0;
    if (fmt == 0x13) {
        peTexFmt = 0xB;
    }
    switch (fmt) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 0x26:
        GX_SET_REG(__GXData->cpTex.word, 3, 15, 16);
        break;
    default:
        GX_SET_REG(__GXData->cpTex.word, 2, 15, 16);
        break;
    }
    __GXData->cpTexZ = (fmt & 0x10) == 0x10;
    GX_SET_REG(__GXData->cpTex.word, (peTexFmt >> 3) & 1, 28, 28);
    __GetImageTileCount(fmt, wd, ht, &rowTiles, &colTiles, &cmpTiles);
    __GXData->cpTexStride.word = 0;
    GX_SET_REG(__GXData->cpTexStride.word, rowTiles * cmpTiles, 22, 31);
    GX_SET_REG(__GXData->cpTexStride.word, 0x4D, 0, 7);
    GX_SET_REG(__GXData->cpTex.word, mipmap, 22, 22);
    GX_SET_REG(__GXData->cpTex.word, peTexFmt, 25, 27);
}

/* Sets the frame-to-field mode of the display copy; the texture copy's is cleared. */
void GXSetDispCopyFrame2Field(s32 mode)
{
    GXData* gx = __GXData;

    GX_SET_REG(gx->cpDisp.word, mode, 18, 19);
    GX_SET_REG(gx->cpTex.word, 0, 18, 19);
}

/* Sets the top and bottom clamp of both copies. */
void GXSetCopyClamp(s32 clamp)
{
    GXData* gx = __GXData;
    u8 clampTop = (clamp & 1) == 1;
    u8 clampBottom = (clamp & 2) == 2;

    GX_SET_REG(gx->cpDisp.word, clampTop, 31, 31);
    GX_SET_REG(gx->cpDisp.word, clampBottom, 30, 30);
    GX_SET_REG(gx->cpTex.word, clampTop, 31, 31);
    GX_SET_REG(gx->cpTex.word, clampBottom, 30, 30);
}

/* Returns the number of XFB lines an `efbHeight`-line copy produces at the scale `iScale` (256 = 1:1), at most 1024. */
static inline u32 __GXGetNumXfbLines(u32 efbHeight, u32 iScale)
{
    u32 count;
    u32 realHt;
    u32 iScaleD;

    count = (efbHeight - 1) * 0x100;
    realHt = count / iScale + 1;
    iScaleD = iScale;
    if (iScaleD > 0x80 && iScaleD < 0x100) {
        while (iScaleD % 2 == 0) {
            iScaleD /= 2;
        }
        if (efbHeight % iScaleD == 0) {
            realHt++;
        }
    }
    if (realHt > 0x400) {
        realHt = 0x400;
    }
    return realHt;
}

/* Returns the vertical scale that copies `efbHeight` EFB lines into as close to `xfbHeight` XFB lines as possible. */
f32 GXGetYScaleFactor(u16 efbHeight, u16 xfbHeight)
{
    f32 fScale;
    f32 yScale;
    u32 iScale;
    u32 tgtHt;
    u32 realHt;

    tgtHt = xfbHeight;
    yScale = (f32)xfbHeight / (f32)efbHeight;
    iScale = (u32)(256.0f / yScale) & 0x1FF;
    realHt = __GXGetNumXfbLines(efbHeight, iScale);

    while (realHt > xfbHeight) {
        tgtHt--;
        yScale = (f32)tgtHt / (f32)efbHeight;
        iScale = (u32)(256.0f / yScale) & 0x1FF;
        realHt = __GXGetNumXfbLines(efbHeight, iScale);
    }

    fScale = yScale;
    while (realHt < xfbHeight) {
        fScale = yScale;
        tgtHt++;
        yScale = (f32)tgtHt / (f32)efbHeight;
        iScale = (u32)(256.0f / yScale) & 0x1FF;
        realHt = __GXGetNumXfbLines(efbHeight, iScale);
    }

    return fScale;
}

/* Sets the vertical scale of the display copy and returns the number of XFB lines it produces. */
u32 GXSetDispCopyYScale(f32 vscale)
{
    u32 iScale;
    u8 enable;
    u32 ht;
    u32 reg;

    iScale = (u32)(256.0f / vscale) & 0x1FF;
    enable = (iScale != 256);

    reg = 0;
    GX_SET_REG(reg, iScale, 23, 31);
    GX_SET_REG(reg, 0x4E, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;
    __GXData->xfWritePending = 0;
    GX_SET_REG(__GXData->cpDisp.word, enable, 21, 21);
    ht = __GXData->cpDispSize.f.y + 1;
    return __GXGetNumXfbLines(ht, iScale);
}

/* Sets the colour and depth an EFB copy with clear writes back. */
void GXSetCopyClear(GXColor clear_clr, u32 clear_z)
{
    u32 reg;

    reg = 0;
    GX_SET_REG(reg, clear_clr.r, 24, 31);
    GX_SET_REG(reg, clear_clr.a, 16, 23);
    GX_SET_REG(reg, 0x4F, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;

    reg = 0;
    GX_SET_REG(reg, clear_clr.b, 24, 31);
    GX_SET_REG(reg, clear_clr.g, 16, 23);
    GX_SET_REG(reg, 0x50, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;

    reg = 0;
    GX_SET_REG(reg, clear_z, 8, 31);
    GX_SET_REG(reg, 0x51, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;
    __GXData->xfWritePending = 0;
}

/* Sets the anti-aliasing sample pattern and the vertical de-flicker filter of the copies. */
void GXSetCopyFilter(u8 aa, const u8 sample_pattern[12][2], u8 vf, const u8 vfilter[7])
{
    u32 msLoc[4];
    u32 coeff0;
    u32 coeff1;

    if (aa) {
        msLoc[0] = 0;
        GX_SET_REG(msLoc[0], sample_pattern[0][0], 28, 31);
        GX_SET_REG(msLoc[0], sample_pattern[0][1], 24, 27);
        GX_SET_REG(msLoc[0], sample_pattern[1][0], 20, 23);
        GX_SET_REG(msLoc[0], sample_pattern[1][1], 16, 19);
        GX_SET_REG(msLoc[0], sample_pattern[2][0], 12, 15);
        GX_SET_REG(msLoc[0], sample_pattern[2][1], 8, 11);
        GX_SET_REG(msLoc[0], 1, 0, 7);

        msLoc[1] = 0;
        GX_SET_REG(msLoc[1], sample_pattern[3][0], 28, 31);
        GX_SET_REG(msLoc[1], sample_pattern[3][1], 24, 27);
        GX_SET_REG(msLoc[1], sample_pattern[4][0], 20, 23);
        GX_SET_REG(msLoc[1], sample_pattern[4][1], 16, 19);
        GX_SET_REG(msLoc[1], sample_pattern[5][0], 12, 15);
        GX_SET_REG(msLoc[1], sample_pattern[5][1], 8, 11);
        GX_SET_REG(msLoc[1], 2, 0, 7);

        msLoc[2] = 0;
        GX_SET_REG(msLoc[2], sample_pattern[6][0], 28, 31);
        GX_SET_REG(msLoc[2], sample_pattern[6][1], 24, 27);
        GX_SET_REG(msLoc[2], sample_pattern[7][0], 20, 23);
        GX_SET_REG(msLoc[2], sample_pattern[7][1], 16, 19);
        GX_SET_REG(msLoc[2], sample_pattern[8][0], 12, 15);
        GX_SET_REG(msLoc[2], sample_pattern[8][1], 8, 11);
        GX_SET_REG(msLoc[2], 3, 0, 7);

        msLoc[3] = 0;
        GX_SET_REG(msLoc[3], sample_pattern[9][0], 28, 31);
        GX_SET_REG(msLoc[3], sample_pattern[9][1], 24, 27);
        GX_SET_REG(msLoc[3], sample_pattern[10][0], 20, 23);
        GX_SET_REG(msLoc[3], sample_pattern[10][1], 16, 19);
        GX_SET_REG(msLoc[3], sample_pattern[11][0], 12, 15);
        GX_SET_REG(msLoc[3], sample_pattern[11][1], 8, 11);
        GX_SET_REG(msLoc[3], 4, 0, 7);
    } else {
        msLoc[0] = 0x01666666;
        msLoc[1] = 0x02666666;
        msLoc[2] = 0x03666666;
        msLoc[3] = 0x04666666;
    }
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = msLoc[0];
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = msLoc[1];
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = msLoc[2];
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = msLoc[3];

    coeff0 = 0;
    GX_SET_REG(coeff0, 0x53, 0, 7);
    coeff1 = 0;
    GX_SET_REG(coeff1, 0x54, 0, 7);
    if (vf) {
        GX_SET_REG(coeff0, vfilter[0], 26, 31);
        GX_SET_REG(coeff0, vfilter[1], 20, 25);
        GX_SET_REG(coeff0, vfilter[2], 14, 19);
        GX_SET_REG(coeff0, vfilter[3], 8, 13);
        GX_SET_REG(coeff1, vfilter[4], 26, 31);
        GX_SET_REG(coeff1, vfilter[5], 20, 25);
        GX_SET_REG(coeff1, vfilter[6], 14, 19);
    } else {
        GX_SET_REG(coeff0, 0, 26, 31);
        GX_SET_REG(coeff0, 0, 20, 25);
        GX_SET_REG(coeff0, 21, 14, 19);
        GX_SET_REG(coeff0, 22, 8, 13);
        GX_SET_REG(coeff1, 21, 26, 31);
        GX_SET_REG(coeff1, 0, 20, 25);
        GX_SET_REG(coeff1, 0, 14, 19);
    }
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = coeff0;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = coeff1;
    __GXData->xfWritePending = 0;
}

/* Sets the gamma correction of the display copy. */
void GXSetDispCopyGamma(s32 gamma)
{
    GX_SET_REG(__GXData->cpDisp.word, gamma, 23, 24);
}

/* Copies the EFB to the external frame buffer `dest`, clearing the EFB afterwards when `clear` is set. */
void GXCopyDisp(void* dest, u8 clear) /* untyped: byte range */
{
    u32 reg;
    u32 tempPeCtrl;
    u8 changePeCtrl;

    if (clear) {
        reg = __GXData->zMode.word;
        GX_SET_REG(reg, 1, 31, 31);
        GX_SET_REG(reg, 7, 28, 30);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = reg;

        reg = __GXData->colorMode.word;
        GX_SET_REG(reg, 0, 31, 31);
        GX_SET_REG(reg, 0, 30, 30);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = reg;
    }

    changePeCtrl = 0;
    if ((clear || (__GXData->pixelCtrl.word & 7) == 3) && ((__GXData->pixelCtrl.word >> 6) & 1) == 1) {
        changePeCtrl = 1;
        tempPeCtrl = __GXData->pixelCtrl.word;
        GX_SET_REG(tempPeCtrl, 0, 25, 25);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = tempPeCtrl;
    }

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpDispSrc.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpDispSize.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpDispStride.word;

    reg = 0;
    GX_SET_REG(reg, (u32)dest >> 5, 8, 31);
    GX_SET_REG(reg, 0x4B, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;

    GX_SET_REG(__GXData->cpDisp.word, clear, 20, 20);
    GX_SET_REG(__GXData->cpDisp.word, 1, 17, 17);
    GX_SET_REG(__GXData->cpDisp.word, 0x52, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpDisp.word;

    if (clear) {
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = __GXData->zMode.word;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = __GXData->colorMode.word;
    }

    if (changePeCtrl) {
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = __GXData->pixelCtrl.word;
    }

    __GXData->xfWritePending = 0;
}

/* Copies the EFB to the texture buffer `dest`, clearing the EFB afterwards when `clear` is set. */
void GXCopyTex(void* dest, u8 clear) /* untyped: byte range */
{
    u32 reg;
    u32 tempPeCtrl;
    u8 changePeCtrl;

    if (clear) {
        reg = __GXData->zMode.word;
        GX_SET_REG(reg, 1, 31, 31);
        GX_SET_REG(reg, 7, 28, 30);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = reg;

        reg = __GXData->colorMode.word;
        GX_SET_REG(reg, 0, 31, 31);
        GX_SET_REG(reg, 0, 30, 30);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = reg;
    }

    changePeCtrl = 0;
    tempPeCtrl = __GXData->pixelCtrl.word;
    if (__GXData->cpTexZ && (tempPeCtrl & 7) != 3) {
        changePeCtrl = 1;
        GX_SET_REG(tempPeCtrl, 3, 29, 31);
    }
    if ((clear || (tempPeCtrl & 7) == 3) && ((tempPeCtrl >> 6) & 1) == 1) {
        changePeCtrl = 1;
        GX_SET_REG(tempPeCtrl, 0, 25, 25);
    }
    if (changePeCtrl) {
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = tempPeCtrl;
    }

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpTexSrc.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpTexSize.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpTexStride.word;

    reg = 0;
    GX_SET_REG(reg, (u32)dest >> 5, 8, 31);
    GX_SET_REG(reg, 0x4B, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg;

    GX_SET_REG(__GXData->cpTex.word, clear, 20, 20);
    GX_SET_REG(__GXData->cpTex.word, 0, 17, 17);
    GX_SET_REG(__GXData->cpTex.word, 0x52, 0, 7);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->cpTex.word;

    if (clear) {
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = __GXData->zMode.word;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = __GXData->colorMode.word;
    }

    if (changePeCtrl) {
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = __GXData->pixelCtrl.word;
    }

    __GXData->xfWritePending = 0;
}

/* Resets the bounding box to empty. */
void GXClearBoundingBox(void)
{
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0x550003FF;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0x560003FF;
    __GXData->xfWritePending = 0;
}
