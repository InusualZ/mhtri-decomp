/*
 * font/dbg_drawgraph.cpp - the game's flat screen-space debug draw: `disp_beta_tex` (a textured quad) and
 *   `dbg_drawgraph_init` (GX set up for untextured vertex-colour drawing), with the SDK's inline GX writers
 *   emitted out of line and the nw4r `ut::Color` word helpers the init uses.
 * RANGE. .text 0x8005ABD8-0x8005B26C (11 functions); extab 0x80007328-0x80007350, extabindex
 *   0x8001F62C-0x8001F668, .sdata2 0x80795CF8-0x80795D20.
 * RANGE. Cut from `font/flfnt.cpp`: `.sdata2` 0x80795CF8-0x80795D20 (0.0f, 1.0f, 0x4330000000000000 among
 *   them) is read only by `disp_beta_tex`/`dbg_drawgraph_init`, and the font console's own pool
 *   0x80795D20-0x80795D48 repeats those three values (a TU pools a value once); the extabindex records split
 *   after the `ut::Color` destructor (0x8001F65C names 0x8005B228+0x44, 0x8001F668 names 0x8005B278); the
 *   inline GX writers are emitted once per TU (here 0x8005AEC0-0x8005AF1C, the console's 0x8005C4CC-0x8005C50C).
 * FLAGS. the `g3d` lib's `cflags_g3d`, as in `font/flfnt.cpp`.
 * NAMES. `disp_beta_tex`/`dbg_drawgraph_init` are the map's.  The `ut::Color` members are the compiler's weak
 *   copies under their manglings (`__ct__Q34nw4r2ut5ColorFUl`, `__as__Q34nw4r2ut5ColorFUl`,
 *   `ToU32ref__Q34nw4r2ut5ColorFv`, `__dt__Q34nw4r2ut5ColorFv`; the g3d result evaluators call the same copies).
 *   GUESS: the unit name (from `dbg_drawgraph_init`), the GX writer copies `dbgGX*` (the SDK's inline GXEnd/
 *   GXTexCoord2f32/GXColor4u8/GXPosition3f32/GXSetTexCoordGen, emitted out of line).
 * RESIDUALS. none.
 * SHAPES. `#pragma peephole off`; `#pragma dont_inline on` from `nw4r/ut_Color.h` on: dbg_drawgraph_init's fog
 *   colour is a named `nw4r::ut::Color` local, so retail's out-of-line constructor call, the argument copy and
 *   the weak copies (constructor, operator=(u32), ToU32ref, destructor, in that order) all come out as retail.
 *   The 0.91346f aspect factor is that literal (0x3F69D884), not 0.9134615f.
 */

#pragma peephole off
#include "types.h"
#include "gx.h"
#include "MTX/mtx.h"
#include "MTX/mtx44.h"
#include "KPAD/kpad.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "Rmode.h"
#include "mh3_pad/Screen_w.h"
#include "rmode_fb_width_get.h"
#include "rmode_efb_height_get.h"
#include "g3d/g3d_state.h"
#include "font/dbg_drawgraph.h"
#pragma dont_inline on
#include "nw4r/ut_Color.h"

/* --- bodies, in address order ---------------------------------------------------------------------- */

extern "C" void dbgGXEnd(void);
extern "C" void dbgGXTexCoord2f32(f32 s, f32 t);
extern "C" void dbgGXColor4u8(u32 r, u32 g, u32 b, u32 a);
extern "C" void dbgGXPosition3f32(f32 x, f32 y, f32 z);
extern "C" void dbgGXSetTexCoordGen(u32 coord, u32 func, u32 src, u32 mtx);

/* 0x8005ABD8 (0x2E8): draws `tex` as a quad at `pos` of `size`, tinted `color`, in screen space. */
void disp_beta_tex(Vec* pos, Vec2* size, GXColor color, GXTexObj* tex)
{
    Mtx44 proj;
    Mtx mv;

    dbg_drawgraph_init();
    GXSetCopyFilter(0, NULL, 0, NULL);
    GXInvalidateTexAll();
    GXSetBlendMode(1, 4, 5, 0);
    GXSetColorUpdate(1);
    C_MTXOrtho(proj, 0.0f, Screen_w.height_f, 0.0f, Screen_w.width_f, 0.0f, 1.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(mv);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
    GXSetCullMode(0);
    GXSetZMode(1, 7, 0);
    GXSetNumTexGens(1);
    dbgGXSetTexCoordGen(0, 1, 4, 60);
    GXLoadTexObj(tex, 0);
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevOp(0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxDesc(13, 1);
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetVtxAttrFmt(0, 13, 1, 4, 0);
    GXBegin(0x80, 0, 4);
    dbgGXPosition3f32(pos->x, pos->y, pos->z);
    dbgGXColor4u8(color.r, color.g, color.b, color.a);
    dbgGXTexCoord2f32(0.0f, 0.0f);
    dbgGXPosition3f32(pos->x + size->x, pos->y, pos->z);
    dbgGXColor4u8(color.r, color.g, color.b, color.a);
    dbgGXTexCoord2f32(1.0f, 0.0f);
    dbgGXPosition3f32(pos->x + size->x, pos->y + size->y, pos->z);
    dbgGXColor4u8(color.r, color.g, color.b, color.a);
    dbgGXTexCoord2f32(1.0f, 1.0f);
    dbgGXPosition3f32(pos->x, pos->y + size->y, pos->z);
    dbgGXColor4u8(color.r, color.g, color.b, color.a);
    dbgGXTexCoord2f32(0.0f, 1.0f);
    dbgGXEnd();
    GXSetCopyFilter(Rmode->aa, Rmode->sample_pattern, 1, Rmode->vfilter);
}

/* The GX pipe helpers (write-gather-pipe writers; the SDK's own `GXPosition*`/`GXColor*` shapes). */
extern "C" void dbgGXEnd(void)
{
}

extern "C" void dbgGXTexCoord2f32(f32 s, f32 t)
{
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

extern "C" void dbgGXColor4u8(u32 r, u32 g, u32 b, u32 a)
{
    GXWGFifo.u8 = (u8)(r & 0xFF);
    GXWGFifo.u8 = (u8)(g & 0xFF);
    GXWGFifo.u8 = (u8)(b & 0xFF);
    GXWGFifo.u8 = (u8)(a & 0xFF);
}

extern "C" void dbgGXPosition3f32(f32 x, f32 y, f32 z)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* A one-texcoord wrapper that pins GX's normalize flag (0) and the identity post-matrix (125). */
extern "C" void dbgGXSetTexCoordGen(u32 coord, u32 func, u32 src, u32 mtx)
{
    GXSetTexCoordGen2(coord, func, src, mtx, 0, 125);
}


/* 0x8005AF1C (0x298): sets GX up for flat untextured screen-space drawing (fog off, vertex colour only). */
void dbg_drawgraph_init(void)
{
    u16 width;
    u16 height;
    f32 w;
    f32 h;
    Mtx44 proj;
    Mtx mv;

    width = rmode_fb_width_get();
    height = rmode_efb_height_get();
    GXSetMisc(1, 8);
    nw4r::ut::Color fog_color(0xFFFFFFFF);
    GXSetFog(0, 0.0f, 0.0f, 0.0f, 0.0f, fog_color);
    GXSetFogRangeAdj(0, 0, NULL);
    w = 0.91346f * width;
    h = height;
    g3d_state_invalidate(4);
    GXSetNumIndStages(0);
    C_MTXOrtho(proj, -0.5f * h, 0.5f * h, -0.5f * w, 0.5f * w, -10.0f, 10.0f);
    GXSetProjection(proj, 1);
    PSMTXIdentity(mv);
    GXLoadPosMtxImm(mv, 0);
    GXSetCurrentMtx(0);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxAttrFmt(0, 9, 0, 4, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevOrder(0, 255, 255, 255);
    GXSetTevColorIn(0, 15, 15, 15, 2);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaIn(0, 7, 7, 7, 1);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    GXSetAlphaCompare(7, 0, 1, 7, 0);
    GXSetBlendMode(1, 4, 5, 5);
    GXSetAlphaUpdate(0);
    GXSetZMode(0, 7, 0);
    GXSetCullMode(2);
}

