/*
 * nw4r/ut_CharWriter.cpp - nw4r::ut::CharWriter, the glyph writer: colours, scale, cursor and the GX setup.
 * RANGE. .text 0x80503314-0x80504A3C (12 functions); .bss 0x80760C98-0x80760CB8 (the fog colour's destructor record
 *   and CharWriter::mLoadingTexture); .sbss 0x80795768-0x80795770 (the fog colour's guard and the colour); .sdata2
 *   0x8079D4F0-0x8079D510.
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File and members from NW4R's ut_CharWriter.cpp: the constructor, SetColorMapping, destructor, SetupGX,
 *   SetFontSize, GetFontWidth/Height/Ascent, Print, PrintGlyph, UpdateVertexColor and the file-local
 *   SetupGXWithColorMapping.
 * RESIDUALS. The constructor calls the out-of-line SetTextColor that retail links from 0x80500844
 *   (`nw4r/db_console.cpp`). The weak Color, ColorMapping, VertexColor and TextColor destructors this object emits
 *   are dropped by the link (the Color one is `font/flfnt.cpp`'s at 0x8005B228).
 * RESIDUALS. flipcheck counts the weak Color destructor against the claim because the map names it; a trial flip
 *   keeps the DOL hash.
 * SHAPES. CharWriter::mLoadingTexture is defined after SetupGX, so the fog colour's destructor record comes first in
 *   `.bss` as in the target.
 * SHAPES. SetupGXForRGBA spells out the default setup instead of calling it (the inline depth orders the fog copies'
 *   stack slots).
 */

#include "nw4r/ut_CharWriter.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"

namespace nw4r {
namespace ut {

/* 0x80503314 (0x220): makes a writer with white text, unit scale, the cursor at the origin and linear filtering. */
CharWriter::CharWriter() : mAlpha(0xFF), mIsWidthFixed(false), mFixedWidth(0.0f), mFont(NULL) {
    mLoadingTexture.Reset();
    ResetColorMapping();
    SetGradationMode(GRADMODE_NONE);
    SetTextColor(Color(0xFFFFFFFF));
    SetScale(1.0f, 1.0f);
    SetCursor(0.0f, 0.0f, 0.0f);
    EnableLinearFilter(true, true);
}

/* 0x80503534 (0x44): sets the colours the glyph intensity is mapped between. */
void CharWriter::SetColorMapping(Color min, Color max) {
    mColorMapping.min = min;
    mColorMapping.max = max;
}

/* 0x80503578 (0x40): destroys the writer. */
CharWriter::~CharWriter() {}

/* The GX state every glyph draw shares: no fog, the identity swap table, one colour channel, one texture
 * coordinate and alpha blending. */
static inline void SetupGXCommon() {
    static const Color fog(0x00000000);

    GXSetFog(0, 0.0f, 0.0f, 0.0f, 0.0f, fog);
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetZTexture(0, 17, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXSetChanCtrl(5, 0, 0, 0, 0, 0, 2);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 60, 0, 125);
    GXSetNumIndStages(0);
    GXSetBlendMode(1, 4, 5, 15);
}

/* The glyph vertex format: float position, RGBA8 colour and u16 texture coordinates. */
static inline void SetupVertexFormat() {
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 11, 1, 5, 0);
    GXSetVtxAttrFmt(0, 13, 1, 2, 15);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(11, 1);
    GXSetVtxDesc(13, 1);
}

static void SetupGXWithColorMapping(Color min, Color max);

/* Intensity textures: the vertex colour, with the texture as alpha. */
static inline void SetupGXForI() {
    SetupGXCommon();
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevColorIn(0, 15, 15, 15, 10);
    GXSetTevAlphaIn(0, 7, 4, 5, 7);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    SetupVertexFormat();
}

/* Any other texture: the texture modulated by the vertex colour. */
static inline void SetupGXDefault() {
    SetupGXCommon();
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0);
    SetupVertexFormat();
}

/* RGBA textures: the texture modulated by the vertex colour, as the default. */
static inline void SetupGXForRGBA() {
    SetupGXCommon();
    GXSetNumTevStages(1);
    GXSetTevDirect(0);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOp(0, 0);
    SetupVertexFormat();
}

/* 0x805035B8 (0xA58): sets GX up to draw this writer's glyphs: colour mapping when one is set, else the setup for
 * the font's texture format. */
void CharWriter::SetupGX() {
    mLoadingTexture.Reset();
    if (mColorMapping.min.ToU32() != 0x00000000 || mColorMapping.max.ToU32() != 0xFFFFFFFF) {
        SetupGXWithColorMapping(mColorMapping.min, mColorMapping.max);
    } else if (mFont != NULL) {
        switch (mFont->GetTextureFormat()) {
        case 0:
        case 1:
            SetupGXForI();
            break;
        case 2:
        case 3:
            SetupGXDefault();
            break;
        case 4:
        case 5:
        case 6:
            SetupGXForRGBA();
            break;
        default:
            SetupGXDefault();
            break;
        }
    } else {
        SetupGXDefault();
    }
}

CharWriter::LoadingTexture CharWriter::mLoadingTexture;

/* 0x80504010 (0xC4): scales the font to draw glyphs `width` by `height` pixels. */
void CharWriter::SetFontSize(f32 width, f32 height) {
    f32 vScale = height / mFont->GetHeight();
    f32 hScale = width / mFont->GetWidth();

    SetScale(hScale, vScale);
}

/* 0x805040D4 (0x60): the scaled font width. */
f32 CharWriter::GetFontWidth() const {
    return mScale.x * mFont->GetWidth();
}

/* 0x80504134 (0x60): the scaled font height. */
f32 CharWriter::GetFontHeight() const {
    return mScale.y * mFont->GetHeight();
}

/* 0x80504194 (0x60): the scaled ascent. */
f32 CharWriter::GetFontAscent() const {
    return mScale.y * mFont->GetAscent();
}

/* 0x805041F4 (0x130): draws `code` at the cursor, advances it and returns the advance. */
f32 CharWriter::Print(u16 code) {
    Glyph glyph;
    f32 width;
    f32 left;

    mFont->GetGlyph(&glyph, code);
    if (mIsWidthFixed) {
        f32 margin = (mFixedWidth - glyph.widths.charWidth * mScale.x) / 2;

        width = mFixedWidth;
        left = margin + glyph.widths.left * mScale.x;
    } else {
        width = glyph.widths.charWidth * mScale.x;
        left = glyph.widths.left * mScale.x;
    }
    PrintGlyph(mCursorPos.x + left, mCursorPos.y, mCursorPos.z, glyph);
    mCursorPos.x += width;
    return width;
}

/* Loads the glyph's sheet into texture `slot` unless it is already there with the same filter. */
inline void CharWriter::LoadTexture(const Glyph& glyph, s32 slot) {
    LoadingTexture loadInfo;

    loadInfo.slot = slot;
    loadInfo.texture = glyph.pTexture;
    loadInfo.filter = mFilter;
    if (loadInfo != mLoadingTexture) {
        GXTexObj texObj;

        GXInitTexObj(&texObj, const_cast<void*>(glyph.pTexture), glyph.texWidth, glyph.texHeight, glyph.texFormat, 0,
                     0, 0);
        GXInitTexObjLOD(&texObj, mFilter.atSmall, mFilter.atLarge, 0.0f, 0.0f, 0.0f, 0, 0, 0);
        GXLoadTexObj(&texObj, slot);
        mLoadingTexture = loadInfo;
    }
}

/* 0x80504324 (0x27C): draws one glyph quad with its top-left corner at (x, y, z). */
void CharWriter::PrintGlyph(f32 x, f32 y, f32 z, const Glyph& glyph) {
    f32 posLeft = x;
    f32 posTop = y;
    f32 posRight = posLeft + glyph.widths.glyphWidth * mScale.x;
    f32 posBottom = posTop + glyph.height * mScale.y;
    f32 posZ = z;
    u16 texLeft = static_cast<u16>((static_cast<u32>(glyph.cellX) << 15) / glyph.texWidth);
    u16 texTop = static_cast<u16>((static_cast<u32>(glyph.cellY) << 15) / glyph.texHeight);
    u16 texRight = static_cast<u16>((static_cast<u32>(glyph.cellX + glyph.widths.glyphWidth) << 15) / glyph.texWidth);
    u16 texBottom = static_cast<u16>((static_cast<u32>(glyph.cellY + glyph.height) << 15) / glyph.texHeight);

    LoadTexture(glyph, 0);
    GXBegin(0x80, 0, 4);

    GXWGFifo.f32 = posLeft;
    GXWGFifo.f32 = posTop;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.lu);
    GXWGFifo.u16 = texLeft;
    GXWGFifo.u16 = texTop;

    GXWGFifo.f32 = posRight;
    GXWGFifo.f32 = posTop;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.ru);
    GXWGFifo.u16 = texRight;
    GXWGFifo.u16 = texTop;

    GXWGFifo.f32 = posRight;
    GXWGFifo.f32 = posBottom;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.rd);
    GXWGFifo.u16 = texRight;
    GXWGFifo.u16 = texBottom;

    GXWGFifo.f32 = posLeft;
    GXWGFifo.f32 = posBottom;
    GXWGFifo.f32 = posZ;
    GXWGFifo.u32 = *reinterpret_cast<const u32*>(&mVertexColor.ld);
    GXWGFifo.u16 = texLeft;
    GXWGFifo.u16 = texBottom;
}

/* 0x805045A0 (0x150): recomputes the corner colours from the text colour, its gradation and the alpha. */
void CharWriter::UpdateVertexColor() {
    mVertexColor.lu = mTextColor.start;
    mVertexColor.ru = (mTextColor.gradationMode != GRADMODE_H) ? mTextColor.start : mTextColor.end;
    mVertexColor.ld = (mTextColor.gradationMode != GRADMODE_V) ? mTextColor.start : mTextColor.end;
    mVertexColor.rd = (mTextColor.gradationMode == GRADMODE_NONE) ? mTextColor.start : mTextColor.end;

    mVertexColor.lu.a = mVertexColor.lu.a * mAlpha / 0xFF;
    mVertexColor.ru.a = mVertexColor.ru.a * mAlpha / 0xFF;
    mVertexColor.ld.a = mVertexColor.ld.a * mAlpha / 0xFF;
    mVertexColor.rd.a = mVertexColor.rd.a * mAlpha / 0xFF;
}

/* Maps the glyph intensity between `min` and `max` in a first TEV stage and modulates by the vertex colour in a
 * second. */
static void SetupGXWithColorMapping(Color min, Color max) {
    SetupGXCommon();
    GXSetNumTevStages(2);
    GXSetTevDirect(0);
    GXSetTevDirect(1);
    GXSetTevSwapMode(0, 0, 0);
    GXSetTevSwapMode(1, 0, 0);
    GXSetTevOrder(0, 0, 0, 0xFF);
    GXSetTevColor(1, min);
    GXSetTevColor(2, max);
    GXSetTevColorIn(0, 2, 4, 8, 15);
    GXSetTevAlphaIn(0, 1, 2, 4, 7);
    GXSetTevColorOp(0, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(0, 0, 0, 0, 1, 0);
    GXSetTevOrder(1, 0xFF, 0xFF, 4);
    GXSetTevColorIn(1, 15, 0, 10, 15);
    GXSetTevAlphaIn(1, 7, 0, 5, 7);
    GXSetTevColorOp(1, 0, 0, 0, 1, 0);
    GXSetTevAlphaOp(1, 0, 0, 0, 1, 0);
    SetupVertexFormat();
}

}  // namespace ut
}  // namespace nw4r
