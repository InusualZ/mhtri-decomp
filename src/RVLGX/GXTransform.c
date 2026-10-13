/*
 * RVLGX/GXTransform.c - the SDK GX transform (projection, matrix loads, viewport, scissor, clip).
 *
 * RANGE. `.text` 0x804BA230-0x804BA9F0; `.sdata2` 0x8079D1F0-0x8079D200 (20 functions / 0x74C B).
 *   - `.sdata2` 0x8079D1F0..0x8079D200 (0.0f, 1.0f, 0.5f, 342.0f) is the file's own pool, read only here
 *     (`callers.py --range`); `GXProject` uses 0.0f/1.0f/0.5f first, `__GXSetViewport` adds 342.0f
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p) plus `-fp_contract off` on this
 *   object: `GXProject` and `__GXSetViewport` are `fmuls`+`fadds` with no `fmadds` in the target (with contraction
 *   on they are 0x144/0x8C B against 0x188/0x90 and fuse every multiply-add); no other row changes with the flag.
 * NAMES. the SDK's names; `GXSetViewportJitter` is a GUESS and `GXGetScissor` is a GUESS (the SDK's scheme: the
 *   viewport setter that shifts by half a line for field 0, the reader of the two scissor corner words); the
 *   static paired-single helpers (`WriteProjPS`, `Copy6Floats`, `WriteMTXPS4x3`, `WriteMTXPS4x2`,
 *   `WriteMTXPS3x3from3x4`) are inlined and leave no symbol; `GXData` and its fields (`RVLGX/GXInit.h`) are named
 *   from their use here.
 * RESIDUALS. none in `.text` or `.sdata2`; the 0x8-byte `.text` tail is link padding.
 * SHAPES. the paired-single copies are `asm volatile` blocks in inlined helpers with register locals (a plain
 *   `asm` block is reordered by the scheduler, and the allocator then reuses one FPR); the register words built
 *   from zero (`GXLoadPosMtxIndx`, `GXLoadNrmMtxIndx3x3`, `GXSetScissorBoxOffset`) use `__rlwimi` through
 *   `GX_SET_REG_FIELD`, which keeps the target's `li 0` + `rlwimi` chain; `GXSetProjectionv` types its ternary as
 *   `GXProjectionType`, which keeps the two `li` arms instead of an `mfcr` set; `GXLoadTexMtxImm` sits under
 *   `#pragma global_optimizer off`, which keeps both `if`/`else` arms (with the optimizer the else values are
 *   hoisted above the branches); `GXGetProjectionv` reloads `__GXData` after the store to `ptr[0]`, as the target does.
 */

#include "RVLGX/GXTransform.h"
#include "RVLGX/GXInit.h"
#include "gx.h"

#define GX_SCISSOR_OFFSET 342

/* Inserts `val` into the `size`-bit field at bit `shift` of `reg` (the SDK's register-field setter). */
#define GX_SET_REG_FIELD(reg, size, shift, val) \
    ((reg) = __rlwimi((reg), (val), (shift), 32 - (shift) - (size), 31 - (shift)))

/* Projects a model-space point through `mtx`, the projection `pm` and the viewport `vp` to screen coordinates. */
void GXProject(f32 x, f32 y, f32 z, const f32 mtx[3][4], const f32* pm, const f32* vp, f32* sx, f32* sy, f32* sz)
{
    f32 peye[3];
    f32 xc;
    f32 yc;
    f32 zc;
    f32 wc;

    peye[0] = mtx[0][3] + ((mtx[0][2] * z) + ((mtx[0][0] * x) + (mtx[0][1] * y)));
    peye[1] = mtx[1][3] + ((mtx[1][2] * z) + ((mtx[1][0] * x) + (mtx[1][1] * y)));
    peye[2] = mtx[2][3] + ((mtx[2][2] * z) + ((mtx[2][0] * x) + (mtx[2][1] * y)));
    if (pm[0] == 0.0f) {
        xc = (peye[0] * pm[1]) + (peye[2] * pm[2]);
        yc = (peye[1] * pm[3]) + (peye[2] * pm[4]);
        zc = pm[6] + (peye[2] * pm[5]);
        wc = 1.0f / -peye[2];
    } else {
        xc = pm[2] + (peye[0] * pm[1]);
        yc = pm[4] + (peye[1] * pm[3]);
        zc = pm[6] + (peye[2] * pm[5]);
        wc = 1.0f;
    }
    *sx = (vp[2] / 2.0f) + (vp[0] + (wc * (xc * vp[2] / 2.0f)));
    *sy = (vp[3] / 2.0f) + (vp[1] + (wc * (-yc * vp[3] / 2.0f)));
    *sz = vp[5] + (wc * (zc * (vp[5] - vp[4])));
}

/* Copies the six projection floats to `dst` as three paired-single pairs. */
static inline void WriteProjPS(register volatile f32* dst, register const f32* src)
{
    register f32 p01;
    register f32 p23;
    register f32 p45;

    asm volatile {
        psq_l  p01, 0(src), 0, 0
        psq_l  p23, 8(src), 0, 0
        psq_l  p45, 16(src), 0, 0
        psq_st p01, 0(dst), 0, 0
        psq_st p23, 0(dst), 0, 0
        psq_st p45, 0(dst), 0, 0
    }
}

/* Copies six floats from `src` to `dst` as three paired-single pairs. */
static inline void Copy6Floats(register f32* dst, register const f32* src)
{
    register f32 p01;
    register f32 p23;
    register f32 p45;

    asm volatile {
        psq_l  p01, 0(src), 0, 0
        psq_l  p23, 8(src), 0, 0
        psq_l  p45, 16(src), 0, 0
        psq_st p01, 0(dst), 0, 0
        psq_st p23, 8(dst), 0, 0
        psq_st p45, 16(dst), 0, 0
    }
}

/* Writes a 3x4 matrix to the FIFO as six paired-single pairs. */
static inline void WriteMTXPS4x3(register volatile f32* dst, register const f32 mtx[][4])
{
    register f32 a00_a01;
    register f32 a02_a03;
    register f32 a10_a11;
    register f32 a12_a13;
    register f32 a20_a21;
    register f32 a22_a23;

    asm volatile {
        psq_l  a00_a01, 0x00(mtx), 0, 0
        psq_l  a02_a03, 0x08(mtx), 0, 0
        psq_l  a10_a11, 0x10(mtx), 0, 0
        psq_l  a12_a13, 0x18(mtx), 0, 0
        psq_l  a20_a21, 0x20(mtx), 0, 0
        psq_l  a22_a23, 0x28(mtx), 0, 0
        psq_st a00_a01, 0(dst), 0, 0
        psq_st a02_a03, 0(dst), 0, 0
        psq_st a10_a11, 0(dst), 0, 0
        psq_st a12_a13, 0(dst), 0, 0
        psq_st a20_a21, 0(dst), 0, 0
        psq_st a22_a23, 0(dst), 0, 0
    }
}

/* Writes the first two rows of a matrix to the FIFO as four paired-single pairs. */
static inline void WriteMTXPS4x2(register volatile f32* dst, register const f32 mtx[][4])
{
    register f32 a00_a01;
    register f32 a02_a03;
    register f32 a10_a11;
    register f32 a12_a13;

    asm volatile {
        psq_l  a00_a01, 0x00(mtx), 0, 0
        psq_l  a02_a03, 0x08(mtx), 0, 0
        psq_l  a10_a11, 0x10(mtx), 0, 0
        psq_l  a12_a13, 0x18(mtx), 0, 0
        psq_st a00_a01, 0(dst), 0, 0
        psq_st a02_a03, 0(dst), 0, 0
        psq_st a10_a11, 0(dst), 0, 0
        psq_st a12_a13, 0(dst), 0, 0
    }
}

/* Writes the 3x3 part of a 3x4 matrix to the FIFO, a pair and a single per row. */
static inline void WriteMTXPS3x3from3x4(register volatile f32* dst, register const f32 mtx[][4])
{
    register f32 a00_a01;
    register f32 a02;
    register f32 a10_a11;
    register f32 a12;
    register f32 a20_a21;
    register f32 a22;

    asm volatile {
        psq_l  a00_a01, 0x00(mtx), 0, 0
        lfs    a02, 0x08(mtx)
        psq_l  a10_a11, 0x10(mtx), 0, 0
        lfs    a12, 0x18(mtx)
        psq_l  a20_a21, 0x20(mtx), 0, 0
        lfs    a22, 0x28(mtx)
        psq_st a00_a01, 0(dst), 0, 0
        stfs   a02, 0(dst)
        psq_st a10_a11, 0(dst), 0, 0
        stfs   a12, 0(dst)
        psq_st a20_a21, 0(dst), 0, 0
        stfs   a22, 0(dst)
    }
}

/* Sends the projection matrix and type to the XF registers. */
void __GXSetProjection(void)
{
    GXWGFifo.u8 = 0x10;
    GXWGFifo.u32 = 0x00061020;
    WriteProjPS(&GXWGFifo.f32, __GXData->projection);
    GXWGFifo.u32 = __GXData->projectionType;
}

/* Sets the projection matrix and type. */
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

/* Sets the projection from a seven-float vector: the type flag, then the six matrix terms. */
void GXSetProjectionv(const f32* ptr)
{
    GXProjectionType type;

    type = (ptr[0] == 0.0f) ? GX_PERSPECTIVE : GX_ORTHOGRAPHIC;
    __GXData->projectionType = type;
    Copy6Floats(__GXData->projection, ptr + 1);
    __GXData->dirtyState |= 0x08000000;
}

/* Reads the projection back as a seven-float vector. */
void GXGetProjectionv(f32* ptr)
{
    GXData* gx = __GXData;

    ptr[0] = (gx->projectionType != GX_PERSPECTIVE) ? 1.0f : 0.0f;
    gx = __GXData;
    Copy6Floats(ptr + 1, gx->projection);
}

/* Loads a 3x4 matrix into the XF position-matrix slot `id`. */
void GXLoadPosMtxImm(const f32 mtx[][4], u32 id)
{
    u32 reg;

    GXWGFifo.u8 = 0x10;
    reg = (id * 4) | 0x000B0000;
    GXWGFifo.u32 = reg;
    WriteMTXPS4x3(&GXWGFifo.f32, mtx);
}

/* Loads the position-matrix slot `id` from the indexed array entry `mtxIndx`. */
void GXLoadPosMtxIndx(u16 mtxIndx, u32 id)
{
    u32 offset;
    u32 reg;

    offset = id * 4;
    reg = 0;
    GX_SET_REG_FIELD(reg, 12, 0, offset);
    GX_SET_REG_FIELD(reg, 4, 12, 11);
    GX_SET_REG_FIELD(reg, 16, 16, mtxIndx);
    GXWGFifo.u8 = 0x20;
    GXWGFifo.u32 = reg;
}

/* Loads the 3x3 part of a 3x4 matrix into the XF normal-matrix slot `id`. */
void GXLoadNrmMtxImm(const f32 mtx[][4], u32 id)
{
    u32 reg;

    reg = (id * 3 + 0x400) | 0x00080000;
    GXWGFifo.u8 = 0x10;
    GXWGFifo.u32 = reg;
    WriteMTXPS3x3from3x4(&GXWGFifo.f32, mtx);
}

/* Loads the normal-matrix slot `id` from the indexed array entry `mtxIndx`. */
void GXLoadNrmMtxIndx3x3(u16 mtxIndx, u32 id)
{
    u32 offset;
    u32 reg;

    offset = id * 3 + 0x400;
    reg = 0;
    GX_SET_REG_FIELD(reg, 12, 0, offset);
    GX_SET_REG_FIELD(reg, 4, 12, 8);
    GX_SET_REG_FIELD(reg, 16, 16, mtxIndx);
    GXWGFifo.u8 = 0x28;
    GXWGFifo.u32 = reg;
}

/* Selects the position matrix the next vertices use. */
void GXSetCurrentMtx(u32 id)
{
    GXData* gx = __GXData;

    gx->matrixIndexLo.f.posMtx = id;
    gx->dirtyState |= 0x04000000;
}

#pragma push
#pragma global_optimizer off
/* Loads a texture matrix (3x4 for type 0, 2x4 otherwise) into the XF texture-matrix slot `id`. */
void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, s32 type)
{
    u32 addr;
    u32 count;
    u32 reg;

    if (id >= 64) {
        addr = (id - 64) * 4 + 0x500;
    } else {
        addr = id * 4;
    }
    if (type == 1) {
        count = 8;
    } else {
        count = 12;
    }
    reg = addr | ((count - 1) << 16);
    GXWGFifo.u8 = 0x10;
    GXWGFifo.u32 = reg;
    if (type == 0) {
        WriteMTXPS4x3(&GXWGFifo.f32, mtx);
    } else {
        WriteMTXPS4x2(&GXWGFifo.f32, mtx);
    }
}
#pragma pop

/* Sends the viewport scale and offset, derived from the stored rectangle and depth range, to the XF registers. */
void __GXSetViewport(void)
{
    GXData* gx = __GXData;
    f32 sx;
    f32 sy;
    f32 sz;
    f32 ox;
    f32 oy;
    f32 oz;
    f32 zmin;
    f32 zmax;

    sx = gx->viewportWidth / 2.0f;
    sy = -gx->viewportHeight / 2.0f;
    ox = 342.0f + (gx->viewportLeft + (gx->viewportWidth / 2.0f));
    oy = 342.0f + (gx->viewportTop + (gx->viewportHeight / 2.0f));
    zmin = gx->viewportNearZ * gx->viewportZScale;
    zmax = gx->viewportFarZ * gx->viewportZScale;
    sz = zmax - zmin;
    oz = zmax + gx->viewportZOffset;
    GXWGFifo.u8 = 0x10;
    GXWGFifo.u32 = 0x0005101A;
    GXWGFifo.f32 = sx;
    GXWGFifo.f32 = sy;
    GXWGFifo.f32 = sz;
    GXWGFifo.f32 = ox;
    GXWGFifo.f32 = oy;
    GXWGFifo.f32 = oz;
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

/* Reads the viewport rectangle and depth range back as six floats. */
void GXGetViewportv(f32* vp)
{
    Copy6Floats(vp, &__GXData->viewportLeft);
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
    u32 reg;
    u32 hx;
    u32 hy;

    reg = 0;
    hx = (u32)(xOffset + GX_SCISSOR_OFFSET) >> 1;
    hy = (u32)(yOffset + GX_SCISSOR_OFFSET) >> 1;
    GX_SET_REG_FIELD(reg, 10, 0, hx);
    GX_SET_REG_FIELD(reg, 10, 10, hy);
    GX_SET_REG_FIELD(reg, 8, 24, 0x59);
    GXWGFifo.u8 = 0x61;
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
