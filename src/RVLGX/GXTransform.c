/*
 * RVLGX/GXTransform.c - the SDK GX transform (projection, matrix loads, viewport, scissor, clip).
 *
 * RANGE. `.text` 0x804BA230-0x804BA9F0; `.sdata2` 0x8079D1F0-0x8079D200 (20 functions / 0x74C B).
 *   - `.sdata2` 0x8079D1F0..0x8079D200 (0.0f, 1.0f, 0.5f, 342.0f) is read by the project / viewport functions; the
 *     first function reads 0.0f/1.0f/0.5f, separating it from the fog pool before it
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. `GXSetViewportJitter` is a GUESS and `GXGetScissor` is a GUESS (the SDK's scheme: the viewport setter that
 *   shifts by half a line for field 0, the reader of the two scissor corner words); `GXData` and
 *   its fields (`RVLGX/GXInit.h`) are named from their use here.
 * RESIDUALS. flipcheck: `.text` is 0x2B8 of 0x7C0 and `.sdata2` 0x4 of 0x10 (the pool entries of the unwritten bodies). Not written: the paired-single bodies (`GXGetProjectionv`, `GXSetProjectionv`, `GXLoadPosMtxImm`,
 *   `GXLoadNrmMtxImm`, `GXLoadTexMtxImm`, `GXGetViewportv`, `__GXSetProjection`, `__GXSetViewport`), the index loads
 *   (`GXLoadPosMtxIndx`, `GXLoadNrmMtxIndx3x3`) and the 0x804BA230 projection helper. Partial: `GXSetClipMode`,
 *   `GXSetScissorBoxOffset`, `__GXSetMatrixIndex` (the target loads `__GXData` between the FIFO byte stores; the
 *   register-field words are plain `u32` here).
 */

#include "RVLGX/GXInit.h"
#include "gx.h"

#define GX_SCISSOR_OFFSET 342

/* 0x804BA3C0 is not written here; the matrix and projection loads below need paired-single stores. */

void GXSetProjection(const f32 mtx[][4], s32 type)
{
    GXData* gx = __GXData;

    gx->projectionType = type;
    gx->projection[0] = mtx[0][0];
    gx->projection[2] = mtx[1][1];
    gx->projection[4] = mtx[2][2];
    gx->projection[5] = mtx[2][3];
    if (type == 1) {
        gx->projection[1] = mtx[0][3];
        gx->projection[3] = mtx[1][3];
    } else {
        gx->projection[1] = mtx[0][2];
        gx->projection[3] = mtx[1][2];
    }
    gx->dirtyState |= 0x08000000;
}

/* Sets the viewport the next draws map to. */
void GXSetViewport(f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ)
{
    GXData* gx = __GXData;

    gx->viewportLeft = left;
    gx->viewportTop = top;
    gx->viewportWidth = width;
    gx->viewportHeight = height;
    gx->viewportNearZ = nearZ;
    gx->viewportFarZ = farZ;
    gx->dirtyState |= 0x10000000;
}

/* Sets the viewport, shifting it half a line up for the even field of an interlaced frame. */
void GXSetViewportJitter(u32 field, f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ)
{
    GXData* gx;

    if (field == 0) {
        top -= 0.5f;
    }
    gx = __GXData;
    gx->viewportLeft = left;
    gx->viewportTop = top;
    gx->viewportWidth = width;
    gx->viewportHeight = height;
    gx->viewportNearZ = nearZ;
    gx->viewportFarZ = farZ;
    gx->dirtyState |= 0x10000000;
}

/* Writes the scissor rectangle to the BP registers. */
void GXSetScissor(u32 left, u32 top, u32 width, u32 height)
{
    GXData* gx = __GXData;
    u32 tp = top + GX_SCISSOR_OFFSET;
    u32 lf = left + GX_SCISSOR_OFFSET;

    gx->scissorTopLeft.f.y = tp;
    gx->scissorTopLeft.f.x = lf;
    gx->scissorBottomRight.f.y = tp + height - 1;
    gx->scissorBottomRight.f.x = lf + width - 1;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->scissorTopLeft.word;
    GXWGFifo.u8 = 0x61;
    GXWGFifo.u32 = gx->scissorBottomRight.word;
    gx->xfWritePending = 0;
}

/* Reads the scissor rectangle back. */
void GXGetScissor(u32* left, u32* top, u32* width, u32* height)
{
    GXData* gx = __GXData;
    u32 topLeft = gx->scissorTopLeft.word;
    u32 bottomRight = gx->scissorBottomRight.word;
    u32 t = topLeft & 0x7FF;
    u32 l = (topLeft >> 12) & 0x7FF;

    *left = l - GX_SCISSOR_OFFSET;
    *top = t - GX_SCISSOR_OFFSET;
    *width = ((bottomRight >> 12) & 0x7FF) - l + 1;
    *height = (bottomRight & 0x7FF) - t + 1;
}

/* Sets the offset of the scissor box in the BP registers. */
void GXSetScissorBoxOffset(s32 xOffset, s32 yOffset)
{
    u32 reg = 0;

    GXWGFifo.u8 = 0x61;
    reg = (reg & ~0x3FF) | (((u32)(xOffset + GX_SCISSOR_OFFSET) >> 1) & 0x3FF);
    reg = (reg & ~0xFFC00) | (((yOffset + GX_SCISSOR_OFFSET) << 9) & 0xFFC00);
    reg = (reg & ~0xFF000000) | 0x59000000;
    GXWGFifo.u32 = reg;
    __GXData->xfWritePending = 0;
}

/* Writes the clip-mode XF register. */
void GXSetClipMode(u32 mode)
{
    GXData* gx;

    GXWGFifo.u8 = 0x10;
    gx = __GXData;
    GXWGFifo.u32 = 0x1005;
    GXWGFifo.u32 = mode;
    gx->xfWritePending = 1;
}

/* Selects the position matrix the next vertices use. */
void GXSetCurrentMtx(u32 id)
{
    GXData* gx = __GXData;

    gx->matrixIndexLo.f.posMtx = id;
    gx->dirtyState |= 0x04000000;
}

/* Flushes the matrix index words to the CP and XF. */
void __GXSetMatrixIndex(s32 index)
{
    GXData* gx;

    if (index < 5) {
        GXWGFifo.u8 = 8;
        gx = __GXData;
        GXWGFifo.u8 = 0x30;
        GXWGFifo.u32 = gx->matrixIndexLo.word;
        GXWGFifo.u8 = 0x10;
        GXWGFifo.u32 = 0x1018;
        GXWGFifo.u32 = gx->matrixIndexLo.word;
    } else {
        GXWGFifo.u8 = 8;
        gx = __GXData;
        GXWGFifo.u8 = 0x40;
        GXWGFifo.u32 = gx->matrixIndexHi;
        GXWGFifo.u8 = 0x10;
        GXWGFifo.u32 = 0x1019;
        GXWGFifo.u32 = gx->matrixIndexHi;
    }
    gx->xfWritePending = 1;
}
