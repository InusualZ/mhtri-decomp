/*
 * RVLGX/GXPixel.c - the SDK GX pixel-engine state (fog, blend, z-mode, pixel format, field mode, display-list call).
 *
 * RANGE. `.text` 0x804B9A30-0x804BA230 (14 functions, `GXSetFog` .. `GXCallDisplayList`); `.data` 0x8061AC98-0x8061ACB8
 *   (`p2f`); `.sdata2` 0x8079D1A0-0x8079D1F0 (the fog constants). All three are read only by this unit's functions.
 *   `GXCallDisplayList` is the SDK's `GXDisplayList.c` function; nothing in the DOL (no `__FILE__` string, no private
 *   pool word, no unwind record) marks a seam before it, so it stays in this range.
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. SDK names throughout (`GXInitFogAdjTable` for 0x804B9C60). GUESS: `p2f` (the SDK's spelling of the
 *   pixel-format table: eight words {0,1,2,3,4,4,4,5}, indexed by the `GXPixelFmt` in `GXSetPixelFmt`). The BP register
 *   unions (`GXColorModeReg`, `GXDstAlphaReg`, `GXZModeReg`, `GXPixelCtrlReg`, `GXGenModeReg`, `GXFogAdjCenterReg`,
 *   `GXLinePtSizeReg`, `GXFieldMaskReg`) are bitfield views of the words `GXData` keeps, from `RVLGX/GXInit.h`.
 * RESIDUALS. none in `.text`/`.data`/`.sdata2`; the `.text` claim ends with 0xC bytes of link padding. The pool labels
 *   are compiler-local (`@NNN`) where the map names them `lbl_8079D1A0`..`lbl_8079D1E8`: same offsets and bytes.
 * SHAPES. `GXSetFog`, `GXSetFogRangeAdj` and the `pixelCtrl` insert of `GXSetPixelFmt` build their words with
 *   `GX_SET_REG` (`__rlwimi`) on a scalar: the bitfield-member spelling of the same inserts allocates the registers the
 *   other way round (and in `GXSetFogRangeAdj` loses the `mr` copy before the opcode insert). `GXSetFog` reads `a` and
 *   `c` as bits through `*(u32*)&`: a `f32`/`u32` union swaps the registers of the two parameter words.
 *   `GXInitFogAdjTable` squares `near_z` inside the loop expression (hoisted by the compiler), and its sqrt(3) is the
 *   SDK's rounded `1.73205f`.
 */

#include "RVLGX/GXPixel.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXGeometry.h"
#include "MSL/sqrt.h"

#define GX_BM_LOGIC 2
#define GX_BM_SUBTRACT 3

/* Inserts `value` into bits `first`..`last` (bit 0 is the most significant) of the register word `word`. */
#define GX_SET_REG(word, value, first, last) ((word) = __rlwimi((word), (value), 31 - (last), (first), (last)))

/* The pixel-engine format each `GXPixelFmt` selects; the three multisample formats share format 4. */
u32 p2f[8] = {0, 1, 2, 3, 4, 4, 4, 5};

/* Sets the fog function, its depth range and color. */
void GXSetFog(u32 type, f32 start_z, f32 end_z, f32 near_z, f32 far_z, GXColor color)
{
    f32 a;
    f32 c;
    GXColorWord rgba;
    u32 fogColor;
    u32 fog0;
    u32 fog1;
    u32 fog2;
    u32 fog3;
    u32 aBits;
    u32 cBits;
    u32 fsel;
    u32 ortho;

    fogColor = 0;
    fog0 = 0;
    fog1 = 0;
    fog2 = 0;
    fog3 = 0;
    fsel = type & 7;
    ortho = (type >> 3) & 1;

    if (ortho) {
        if (far_z == near_z || end_z == start_z) {
            a = 0.0f;
            c = 0.0f;
        } else {
            f32 scale = 1.0f / (end_z - start_z);
            a = scale * (far_z - near_z);
            c = scale * (start_z - near_z);
        }
    } else {
        f32 A;
        f32 B;
        f32 C;
        s32 expB;
        u32 bMag;
        u32 bShift;

        if (far_z == near_z || end_z == start_z) {
            A = 0.0f;
            B = 0.5f;
            C = 0.0f;
        } else {
            A = far_z * near_z / ((far_z - near_z) * (end_z - start_z));
            B = far_z / (far_z - near_z);
            C = start_z / (end_z - start_z);
        }
        expB = 0;
        while (B > 1.0) {
            B /= 2.0f;
            expB++;
        }
        while (B > 0.0f && B < 0.5) {
            B *= 2.0f;
            expB--;
        }
        a = A / (f32)(1 << (expB + 1));
        bMag = (u32)(8388638.0f * B);
        bShift = expB + 1;
        c = C;
        GX_SET_REG(fog1, bMag, 8, 31);
        GX_SET_REG(fog1, 0xEF, 0, 7);
        GX_SET_REG(fog2, bShift, 27, 31);
        GX_SET_REG(fog2, 0xF0, 0, 7);
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = fog1;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = fog2;
    }

    aBits = *(u32*)&a;
    cBits = *(u32*)&c;
    GX_SET_REG(fog0, aBits >> 12, 21, 31);
    GX_SET_REG(fog0, aBits >> 23, 13, 20);
    GX_SET_REG(fog0, aBits >> 31, 12, 12);
    GX_SET_REG(fog0, 0xEE, 0, 7);

    GX_SET_REG(fog3, cBits >> 12, 21, 31);
    GX_SET_REG(fog3, cBits >> 23, 13, 20);
    GX_SET_REG(fog3, cBits >> 31, 12, 12);
    GX_SET_REG(fog3, ortho, 11, 11);
    GX_SET_REG(fog3, fsel, 8, 10);
    GX_SET_REG(fog3, 0xF1, 0, 7);

    rgba.color = color;
    GX_SET_REG(fogColor, rgba.word >> 8, 8, 31);
    GX_SET_REG(fogColor, 0xF2, 0, 7);

    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = fog0;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = fog3;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = fogColor;
    __GXData->xfWritePending = 0;
}

/* Fills the fog range-adjust table for a viewport `width` pixels wide and the projection `projmtx`. */
void GXInitFogAdjTable(GXFogAdjTable* table, u16 width, const f32 projmtx[4][4])
{
    f32 xi;
    f32 iw;
    f32 range;
    f32 near_z;
    f32 side_x;
    u32 i;

    if (projmtx[3][3] == 0.0) {
        near_z = projmtx[2][3] / (projmtx[2][2] - 1.0f);
        side_x = near_z / projmtx[0][0];
    } else {
        side_x = 1.0f / projmtx[0][0];
        near_z = 1.73205f * side_x;
    }
    iw = 2.0f / width;
    for (i = 0; i < 10; i++) {
        xi = (i + 1) << 5;
        xi *= iw;
        xi *= side_x;
        range = sqrt(1.0f + xi * xi / (near_z * near_z));
        table->r[i] = (u32)(256.0f * range) & 0xFFF;
    }
}

/* Sets the fog range adjustment table and the center of the screen. */
void GXSetFogRangeAdj(u8 enable, u16 center, const GXFogAdjTable* table)
{
    u32 adj;
    GXFogAdjCenterReg reg;
    u32 i;

    if (enable) {
        for (i = 0; i < 10; i += 2) {
            adj = 0;
            GX_SET_REG(adj, table->r[i], 20, 31);
            GX_SET_REG(adj, table->r[i + 1], 8, 19);
            GX_SET_REG(adj, (i >> 1) + 0xE9, 0, 7);
            GXWGFifo.u8 = 0x61;
            GXWGFifo.u32 = adj;
        }
    }
    reg.word = 0;
    reg.f.center = center + 342;
    reg.f.enable = enable;
    reg.f.opcode = 0xE8;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    __GXData->xfWritePending = 0;
}

/* Sets the blend mode, factors and logic operation. */
void GXSetBlendMode(u32 type, u32 src_factor, u32 dst_factor, u32 op)
{
    GXData* gx = __GXData;
    GXColorModeReg reg = gx->colorMode;

    reg.f.subtract = (type == GX_BM_SUBTRACT);
    reg.f.blendEnable = type & 1;
    reg.f.logicEnable = (type == GX_BM_LOGIC);
    reg.f.logicOp = op;
    reg.f.srcFactor = src_factor;
    reg.f.dstFactor = dst_factor;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->colorMode = reg;
    gx->xfWritePending = 0;
}

/* Enables or disables the color buffer write. */
void GXSetColorUpdate(u32 enable)
{
    GXData* gx = __GXData;
    GXColorModeReg reg = gx->colorMode;

    reg.f.colorUpdate = enable;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->colorMode = reg;
    gx->xfWritePending = 0;
}

/* Enables or disables the alpha buffer write. */
void GXSetAlphaUpdate(u32 enable)
{
    GXData* gx = __GXData;
    GXColorModeReg reg = gx->colorMode;

    reg.f.alphaUpdate = enable;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->colorMode = reg;
    gx->xfWritePending = 0;
}

/* Sets the depth test, compare function and depth write. */
void GXSetZMode(u8 compare_enable, u32 func, u8 update_enable)
{
    GXData* gx = __GXData;
    GXZModeReg reg = gx->zMode;

    reg.f.enable = compare_enable;
    reg.f.func = func;
    reg.f.update = update_enable;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->zMode = reg;
    gx->xfWritePending = 0;
}

/* Selects whether the depth compare happens before or after texturing. */
void GXSetZCompLoc(u8 before_tex)
{
    GXData* gx = __GXData;

    gx->pixelCtrl.f.zCompLoc = before_tex;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->pixelCtrl.word;
    gx->xfWritePending = 0;
}

/* Sets the pixel and depth formats, and the antialiasing mode they imply. */
void GXSetPixelFmt(s32 pix_fmt, s32 z_fmt)
{
    GXData* gx = __GXData;
    u32 old = gx->pixelCtrl.word;

    GX_SET_REG(gx->pixelCtrl.word, p2f[pix_fmt], 29, 31);
    GX_SET_REG(gx->pixelCtrl.word, z_fmt, 26, 28);
    if (old != gx->pixelCtrl.word) {
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = gx->pixelCtrl.word;
        gx->genMode.f.multisample = (pix_fmt == 2);
        gx->dirtyState |= 4;
    }
    if (p2f[pix_fmt] == 4) {
        gx->dstAlpha.f.format = pix_fmt - 4;
        gx->dstAlpha.f.opcode = 0x42;
        GXWGFifo.u8 = 0x61;
        GXWGFifo.u32 = gx->dstAlpha.word;
    }
    gx->xfWritePending = 0;
}

/* Enables or disables the dither. */
void GXSetDither(u32 enable)
{
    GXData* gx = __GXData;
    GXColorModeReg reg = gx->colorMode;

    reg.f.dither = enable;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->colorMode = reg;
    gx->xfWritePending = 0;
}

/* Sets the constant destination alpha. */
void GXSetDstAlpha(u32 enable, u32 alpha)
{
    GXData* gx = __GXData;
    GXDstAlphaReg reg = gx->dstAlpha;

    reg.f.alpha = alpha;
    reg.f.enable = enable;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    gx->dstAlpha = reg;
    gx->xfWritePending = 0;
}

/* Masks the even and odd field writes of the copy. */
void GXSetFieldMask(u32 odd_mask, u32 even_mask)
{
    GXFieldMaskReg reg;

    reg.word = 0;
    reg.f.even = even_mask;
    reg.f.odd = odd_mask;
    reg.f.opcode = 0x44;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    __GXData->xfWritePending = 0;
}

/* Sets the field rendering mode and the half aspect ratio of the line and point sizes. */
void GXSetFieldMode(u8 field_mode, u8 half_aspect)
{
    GXData* gx = __GXData;

    gx->lpSize.f.halfAspect = half_aspect;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->lpSize.word;
    __GXFlushTextureState();
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0x68000000 | field_mode;
    __GXFlushTextureState();
}

/* Sends a display list of `size` bytes to the command processor. */
void GXCallDisplayList(const void* list, u32 size) /* untyped: byte range */
{
    GXData* gx = __GXData;

    if (gx->dirtyState) {
        __GXSetDirtyState();
    }
    if (gx->vtxSentWord == 0) {
        __GXSendFlushPrim();
    }
    GXWGFifo.u8 = 0x40;
    GXWGFifo.u32 = (u32)list;
    GXWGFifo.u32 = size;
}
