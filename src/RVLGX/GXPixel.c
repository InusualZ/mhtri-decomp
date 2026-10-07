/*
 * RVLGX/GXPixel.c - the SDK GX pixel-engine state (fog, blend, z-mode, pixel format, display-list call).
 *
 * RANGE. `.text` 0x804B9A30-0x804BA230; `.data` 0x8061AC98-0x8061ACB8; `.sdata2` 0x8079D1A0-0x8079D1F0 (14 functions
 *   / 0x7C4 B).
 *   - `.data` 0x8061AC98 (the pixel-format table) and `.sdata2` 0x8079D1A0..0x8079D1F0 (the fog constants) are read
 *     only here
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. the BP register unions (`GXColorModeReg`, `GXDstAlphaReg`, `GXZModeReg`, `GXPixelCtrlReg`, `GXFogAdjReg`,
 *   `GXFogAdjCenterReg`, `GXFieldModeReg` in `RVLGX/GXInit.h`) are bitfield views of the words `GXData` keeps: a
 *   bitfield member store is the only spelling that gives the target's bare `rlwimi` inserts.
 * RESIDUALS. flipcheck: `.sdata2` 0x50 is claimed but not emitted until the fog bodies are written. `GXSetFogRangeAdj`: the target builds each table word in a scratch register and copies it (`mr`) before
 *   the opcode insert, and orders the loads differently. Not written: `GXSetFog`, the fog-range helper at 0x804B9C60,
 *   `GXSetPixelFmt` (reads the `.data` table at 0x8061AC98), `GXSetFieldMode`,
 *   `GXCallDisplayList` (calls `__GXSetDirtyState` and `__GXSendFlushPrim`, unnamed in other units).
 */

#include "RVLGX/GXPixel.h"
#include "RVLGX/GXInit.h"

#define GX_BM_LOGIC 2
#define GX_BM_SUBTRACT 3

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

/* Writes one fog range-adjust table word. */
static inline void GXWriteFogAdj(u32 opcode, u16 low, u16 high)
{
    GXFogAdjReg reg;

    reg.word = 0;
    reg.f.low = low;
    reg.f.high = high;
    reg.f.opcode = opcode;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
}

/* Sets the fog range adjustment table and the center of the screen. */
void GXSetFogRangeAdj(u8 enable, u16 center, const GXFogAdjTable* table)
{
    GXFogAdjCenterReg reg;

    if (enable) {
        GXWriteFogAdj(0xE9, table->r[0], table->r[1]);
        GXWriteFogAdj(0xEA, table->r[2], table->r[3]);
        GXWriteFogAdj(0xEB, table->r[4], table->r[5]);
        GXWriteFogAdj(0xEC, table->r[6], table->r[7]);
        GXWriteFogAdj(0xED, table->r[8], table->r[9]);
    }
    reg.word = 0;
    reg.f.center = center + 342;
    reg.f.enable = enable;
    reg.f.opcode = 0xE8;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = reg.word;
    __GXData->xfWritePending = 0;
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
