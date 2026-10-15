/*
 * homebutton/hbm_gui_draw.cpp - the band's two GX draw helpers: a filled quad scaled about its centre and a thick
 * line drawn as a quad.
 *
 * RANGE.     .text 0x80569E64..0x8056A478 (2 functions); .sdata2 0x8079D788..0x8079D7A0 (their float pool).
 * FLAGS.     GC/3.0a5.2 + cflags_hkb (configure.py); the retail object has no extab/extabindex, so `#pragma exceptions off`.
 * NAMES.     GUESS file name hbm_gui_draw; GUESS hbmGuiDrawQuad (GX_QUADS fill scaled about the rectangle's centre) and
 *            hbmGuiDrawLine (a quad of width/6 around a vertical or horizontal segment, after GXSetLineWidth).
 * SHAPES.    A TU of its own: homebutton/gui.cpp's adjustor thunks (emitted at the end of their TU) sit before
 *            hbmGuiDrawQuad in retail, and only these two functions read the .sdata2 pool.
 * SHAPES.    hbmGuiDrawQuad: the four vertices are separate locals declared v3..v0 and the matrices trans, scale,
 *            identity (MWCC places same-size locals in reverse declaration order, smaller ones first), and the
 *            centre is `left + (right - left) / 2.0f` (retail's fmuls operand order).
 * RESIDUALS. hbmGuiDrawLine: retail reloads the colour word before each of the four FIFO writes (`lwz r0,0(r31)`),
 *            ours loads it once per branch (162 vs 168 instructions); by-value/by-reference/pointer colour
 *            parameters, a per-vertex inline, an address-bound FIFO object and optimization pragmas all keep the CSE.
 * RESIDUALS. .sdata2: retail orders the pool 0.0, 0.5, 6.0, bias; ours 0.5, 0.0, 6.0, bias (first use in
 *            hbmGuiDrawQuad is the 0.5).
 */

#include "types.h"
#include "sys_mem.h"

/* The retail object has no extab/extabindex: exceptions are off for this band. */
#pragma exceptions off

#include "homebutton/hbm_gui_draw.h"
#include "MTX/mtx.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTransform.h"

/* One vertex of hbmGuiDrawQuad's local quad: a position and the colour components as words.  size: 0x1C */
struct HbmGuiVertex {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
    /* +0x0C */ u32 r;
    /* +0x10 */ u32 g;
    /* +0x14 */ u32 b;
    /* +0x18 */ u32 a;
}; /* size: 0x1C */

/* Writes one position to the GX FIFO. */
static inline void hbmGuiPosition3f32(f32 x, f32 y, f32 z)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Writes one colour as four bytes to the GX FIFO. */
static inline void hbmGuiColor4u8(u8 r, u8 g, u8 b, u8 a)
{
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

/* Writes one colour as a word to the GX FIFO. */
static inline void hbmGuiColor1u32(u32 color)
{
    GXWGFifo.u32 = color;
}

/* Sets up the one-colour pass-through TEV state both helpers draw with. */
static inline void hbmGuiSetupColorQuad()
{
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetCullMode(0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 1, 1, 0, 0, 2);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 4);
    GXSetTevOrder(0, 0xFF, 0xFF, 4);
    GXSetBlendMode(1, 4, 5, 15);
}

extern "C" {

/* 0x80569E64 (0x374): draws a filled quad over the rectangle, scaled by `scale` about its centre at depth z. */
void hbmGuiDrawQuad(const GXColor& color, f32 left, f32 top, f32 right, f32 bottom, f32 z, f32 scale)
{
    Mtx transMtx;
    Mtx scaleMtx;
    Mtx identity;
    HbmGuiVertex v3;
    HbmGuiVertex v2;
    HbmGuiVertex v1;
    HbmGuiVertex v0;
    f32 cx;
    f32 cy;

    hbmGuiSetupColorQuad();
    cx = left + (right - left) / 2.0f;
    cy = top + (bottom - top) / 2.0f;
    PSMTXScale(scaleMtx, scale, scale, scale);
    PSMTXTrans(transMtx, cx, cy, z);
    PSMTXConcat(transMtx, scaleMtx, transMtx);
    GXLoadPosMtxImm(transMtx, 0);

    v0.x = right - cx;
    v0.y = bottom - cy;
    v0.z = 0.0f;
    v0.r = color.r;
    v0.g = color.g;
    v0.b = color.b;
    v0.a = color.a;
    v1.x = left - cx;
    v1.y = bottom - cy;
    v1.z = 0.0f;
    v1.r = color.r;
    v1.g = color.g;
    v1.b = color.b;
    v1.a = color.a;
    v2.x = right - cx;
    v2.y = top - cy;
    v2.z = 0.0f;
    v2.r = color.r;
    v2.g = color.g;
    v2.b = color.b;
    v2.a = color.a;
    v3.x = left - cx;
    v3.y = top - cy;
    v3.z = 0.0f;
    v3.r = color.r;
    v3.g = color.g;
    v3.b = color.b;
    v3.a = color.a;

    GXBegin(0x80, 0, 4);
    hbmGuiPosition3f32(v3.x, v3.y, v3.z);
    hbmGuiColor4u8(v3.r, v3.g, v3.b, v3.a);
    hbmGuiPosition3f32(v1.x, v1.y, v1.z);
    hbmGuiColor4u8(v1.r, v1.g, v1.b, v1.a);
    hbmGuiPosition3f32(v0.x, v0.y, v0.z);
    hbmGuiColor4u8(v0.r, v0.g, v0.b, v0.a);
    hbmGuiPosition3f32(v2.x, v2.y, v2.z);
    hbmGuiColor4u8(v2.r, v2.g, v2.b, v2.a);

    PSMTXIdentity(identity);
    GXLoadPosMtxImm(identity, 0);
    GXSetCurrentMtx(0);
}

/* 0x8056A1D8 (0x2A0): draws a vertical (left == right) or horizontal segment as a quad width/6 thick at depth z. */
void hbmGuiDrawLine(u8 width, const GXColor& color, f32 left, f32 top, f32 right, f32 bottom, f32 z)
{
    Mtx mtx;
    f32 half;

    hbmGuiSetupColorQuad();
    PSMTXTrans(mtx, 0.0f, 0.0f, 0.0f);
    GXLoadPosMtxImm(mtx, 0);
    GXSetLineWidth(width, 0);
    half = (f32)width / 6.0f / 2.0f;
    if (left == right) {
        GXBegin(0x80, 0, 4);
        hbmGuiPosition3f32(left - half, top, z);
        hbmGuiColor1u32(*(const u32*)&color);
        hbmGuiPosition3f32(left - half, bottom, z);
        hbmGuiColor1u32(*(const u32*)&color);
        hbmGuiPosition3f32(right + half, bottom, z);
        hbmGuiColor1u32(*(const u32*)&color);
        hbmGuiPosition3f32(right + half, top, z);
        hbmGuiColor1u32(*(const u32*)&color);
    } else {
        GXBegin(0x80, 0, 4);
        hbmGuiPosition3f32(left, top - half, z);
        hbmGuiColor1u32(*(const u32*)&color);
        hbmGuiPosition3f32(left, bottom + half, z);
        hbmGuiColor1u32(*(const u32*)&color);
        hbmGuiPosition3f32(right, bottom + half, z);
        hbmGuiColor1u32(*(const u32*)&color);
        hbmGuiPosition3f32(right, top - half, z);
        hbmGuiColor1u32(*(const u32*)&color);
    }
}

}
