/*
 * RVLGX/GXGeometry.c - the SDK GX geometry state (dirty-state flush, `GXBegin`, line/point/cull setters).
 *
 * RANGE. `.text` 0x804B65F0-0x804B6C00 (9 functions, `__GXSetDirtyState` .. `__GXSetGenMode`; 0x5D0 B of functions,
 *   0x610 B claimed with alignment and trailing padding). No data of its own: every function reads only `__GXData`
 *   (`RVLGX/GXInit.c`). The run is bounded by the `GXMisc` range and the first `GXFrameBuf` function; the extent
 *   follows the SDK function order.
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, -inline auto). No float op in
 *   the target, so no `fp_contract` evidence either way.
 * NAMES. SDK names throughout, including the SDK file-static helpers `__GXSetLightChan` and `__GXSetTexGen`. GUESS:
 *   `WriteGenMode` and `SendFlushPrim` are local inline copies of `__GXSetGenMode` and `__GXSendFlushPrim`.
 * RESIDUALS. `__GXSetDirtyState` only: the chan loop emits `addi r5,r31,0xB8` + `lwz 0(r5)` where the target has
 *   `mr r5,r31` + `lwz 0xB8(r5)` (the indexed `chanCtrl[i]` spelling gives that pair but scores lower); the texgen
 *   loop colours `bits`/the walk r7/r8 where the target has r8/r7. Register colouring; the indexed walk is numbered
 *   last in the front end.
 * SHAPES. The target inlines the genMode write into `__GXSetDirtyState` and the flush-primitive body into `GXBegin`
 *   though both are defined later; `-inline auto` only inlines an earlier definition, so the bodies are static inline
 *   helpers above their callers. `__GXSendFlushPrim` keeps its own copy of the body (a call to the helper inlines into
 *   a shorter function). `GXSetCullMode` builds the swapped front/back bits as `(mode << 1) & 2` then inserts
 *   `(mode >> 1) & 1`: the `__rlwimi` spelling of the same pair allocates its two registers the other way round. The
 *   numColorChans/numTexGens values are read into a local before the XF write, which lets the scheduler hoist the
 *   `genMode` load as the target does.
 */

#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"

#define XF_WRITE(addr, val) (GXWGFifo.u8 = 0x10, GXWGFifo.u32 = (addr), GXWGFifo.u32 = (val))

/* Sends the cached genMode word to the BP and marks the last write as a BP write. */
static inline void WriteGenMode(void)
{
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = __GXData->genMode.word;
    __GXData->xfWritePending = 0;
}

/* Sends an empty primitive of `vNum` vertices of `vLim` bytes to flush the vertex cache. */
static inline void SendFlushPrim(void)
{
    u32 i;
    u32 numD = __GXData->vNum * __GXData->vLim;
    GXWGFifo.u8 = 0x98;
    GXWGFifo.u16 = __GXData->vNum;
    for (i = 0; i < numD; i += 4) {
        GXWGFifo.u32 = 0;
    }
    __GXData->xfWritePending = 1;
}

/* Sends the dirty channel count and channel controls to the XF. */
static inline void __GXSetLightChan(GXData* gx, u32 dirty)
{
    u32 bits;
    u32 addr = 0x100E;
    if (dirty & 0x01000000) {
        u32 nChan = gx->genMode.f.numColorChans;
        XF_WRITE(0x1009, nChan);
    }
    {
        GXChanCtrlReg* ctrl = gx->chanCtrl;
        for (bits = (dirty >> 12) & 0xF; bits != 0; ctrl++, addr++, bits >>= 1) {
            if (bits & 1) XF_WRITE(addr, ctrl->word);
        }
    }
}

/* Sends the dirty texgen count, texgen controls and post-texgen controls to the XF. */
static inline void __GXSetTexGen(GXData* gx, u32 dirty)
{
    u32 i;
    u32 bits;
    u32 taddr = 0x1040;
    if (dirty & 0x02000000) {
        u32 nTex = gx->genMode.f.numTexGens;
        XF_WRITE(0x103F, nTex);
    }
    bits = (dirty >> 16) & 0xFF;
    for (i = 0; bits != 0; taddr++, i++, bits >>= 1) {
        u32 ptaddr = taddr + 0x10;
        if (bits & 1) {
            XF_WRITE(taddr, gx->texGenCtrl[i].word);
            XF_WRITE(ptaddr, gx->postTexGen[i].word);
        }
    }
}

/* 0x804B65F0 (0x280): flushes every register group marked in `dirtyState` and clears it. */
void __GXSetDirtyState(void)
{
    GXData* gx = __GXData;
    u32 dState = gx->dirtyState;
    u32 lightBits;
    u32 texBits;

    if (dState & 1) __GXSetSUTexRegs();
    if (dState & 2) __GXUpdateBPMask();
    if (dState & 4) WriteGenMode();
    if (dState & 8) __GXSetVCD();
    if (dState & 0x10) __GXSetVAT();
    if (dState & 0x18) __GXCalculateVLim();
    dState &= 0xFFFFFF00;
    if (dState) {
        u32 amb = dState & 0xF00;
        if (amb) {
            if (amb & 0x100) XF_WRITE(0x100A, gx->ambColor[0].word);
            if (amb & 0x200) XF_WRITE(0x100B, gx->ambColor[1].word);
            if (amb & 0x400) XF_WRITE(0x100C, gx->matColor[0].word);
            if (amb & 0x800) XF_WRITE(0x100D, gx->matColor[1].word);
        }
        lightBits = dState & 0x0100F000;
        if (lightBits) __GXSetLightChan(gx, lightBits);
        texBits = dState & 0x02FF0000;
        if (texBits) __GXSetTexGen(gx, texBits);
        if (dState & 0x04000000) {
            __GXSetMatrixIndex(0);
            __GXSetMatrixIndex(5);
        }
        if (dState & 0x10000000) __GXSetViewport();
        if (dState & 0x08000000) __GXSetProjection();
        gx->xfWritePending = 1;
    }
    gx->dirtyState = 0;
}

/* 0x804B6870 (0x14C): flushes dirty state and opens a primitive of `nverts` vertices. */
void GXBegin(u32 type, u32 vtxfmt, u16 nverts)
{
    GXData* gx = __GXData;
    if (gx->dirtyState) __GXSetDirtyState();
    if (gx->vtxSentWord == 0) SendFlushPrim();
    GXWGFifo.u8 = vtxfmt | type;
    GXWGFifo.u16 = nverts;
}

/* 0x804B69C0 (0xD8): sends an empty primitive to flush the vertex cache. */
void __GXSendFlushPrim(void)
{
    u32 i;
    u32 numD = __GXData->vNum * __GXData->vLim;
    GXWGFifo.u8 = 0x98;
    GXWGFifo.u16 = __GXData->vNum;
    for (i = 0; i < numD; i += 4) {
        GXWGFifo.u32 = 0;
    }
    __GXData->xfWritePending = 1;
}

/* 0x804B6AA0 (0x34): sets the line width and its texture offset scale. */
void GXSetLineWidth(u8 width, u32 tex_offsets)
{
    GXData* gx = __GXData;
    GX_SET_REG(gx->lpSize.word, width, 24, 31);
    GX_SET_REG(gx->lpSize.word, tex_offsets, 13, 15);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->lpSize.word;
    gx->xfWritePending = 0;
}

/* 0x804B6AE0 (0x34): sets the point size and its texture offset scale. */
void GXSetPointSize(u8 size, u32 tex_offsets)
{
    GXData* gx = __GXData;
    GX_SET_REG(gx->lpSize.word, size, 16, 23);
    GX_SET_REG(gx->lpSize.word, tex_offsets, 10, 12);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->lpSize.word;
    gx->xfWritePending = 0;
}

/* 0x804B6B20 (0x3C): enables the line and point texture offsets of one texture coordinate. */
void GXEnableTexOffsets(u32 coord, u8 line_enable, u8 point_enable)
{
    GXData* gx = __GXData;
    GX_SET_REG(gx->suTs0[coord].word, line_enable, 13, 13);
    GX_SET_REG(gx->suTs0[coord].word, point_enable, 12, 12);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->suTs0[coord].word;
    gx->xfWritePending = 0;
}

/* 0x804B6B60 (0x28): stores the cull mode with front and back swapped into genMode. */
void GXSetCullMode(u32 mode)
{
    GXData* gx = __GXData;
    u32 hwMode = ((mode << 1) & 2);
    hwMode = (hwMode & ~1) | ((mode >> 1) & 1);
    GX_SET_REG(gx->genMode.word, hwMode, 16, 17);
    gx->dirtyState |= 4;
}

/* 0x804B6B90 (0x3C): sets the co-planar bit of genMode and sends it through a masked BP write. */
void GXSetCoPlanar(u8 enable)
{
    GXData* gx = __GXData;
    GX_SET_REG(gx->genMode.word, enable, 12, 12);
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = 0xFE080000;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->genMode.word;
    gx->xfWritePending = 0;
}

/* 0x804B6BD0 (0x24): sends the cached genMode word to the BP. */
void __GXSetGenMode(void)
{
    WriteGenMode();
}
