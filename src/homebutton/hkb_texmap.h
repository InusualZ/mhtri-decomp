/*
 * homebutton/hkb_texmap.h - the keyboard layout engine's texture map (nw4r::lyt::TexMap): a texture image, its
 *   palette and the GX sampling state, packed so homebutton/hkb_material.cpp can hold one per texture slot.
 *   The layout is read from Get, Set and ReplaceImage in homebutton/hkb_texmap.cpp.
 */
#ifndef HOMEBUTTON_HKB_TEXMAP_H
#define HOMEBUTTON_HKB_TEXMAP_H

#include "types.h"
#include "gx.h"

struct TPLPalette;

namespace nw4r {
namespace lyt {

namespace detail {

/* Returns whether a GX texel format is one of the colour-index formats (C4, C8, C14X2). */
inline bool IsCITexelFormat(s32 fmt) {
    return (u32)(fmt - 8) <= 2;
}

}  // namespace detail

/* A texture bound to GX: the image, its palette, the sampling state. size: 0x1C */
class TexMap {
public:
    TexMap() {
        mpImage = NULL;
        mWidth = 0;
        mHeight = 0;
        mBits.textureFormat = 0;
        mBits.mipmap = false;
        mBits.wrapS = 0;
        mBits.wrapT = 0;
        mBits.minFilter = 1;
        mBits.magFilter = 1;
        mMinLOD = 0.0f;
        mMaxLOD = 0.0f;
        mLODBias = 0;
        mBits.biasClampEnable = false;
        mBits.edgeLODEnable = false;
        mBits.anisotropy = 0;
        mpPalette = NULL;
        mBits.paletteFormat = 0;
        mPaletteEntryNum = 0;
    }

    /* Takes the image and sampling state of a GX texture object, with no palette. */
    TexMap(const GXTexObj& texObj) {
        Set(texObj);
        mpPalette = NULL;
        mBits.paletteFormat = 0;
        mPaletteEntryNum = 0;
    }

    void Get(GXTexObj* pTexObj) const;
    void Get(GXTlutObj* pTlutObj) const;
    void Set(const GXTexObj& texObj);
    void ReplaceImage(TPLPalette* pTplRes, u32 id);

    /* untyped: the texel bytes GX samples */
    void* GetImage() const { return mpImage; }
    /* untyped: the TLUT bytes GX looks colour indices up in */
    void* GetPalette() const { return mpPalette; }
    s32 GetTexelFormat() const { return mBits.textureFormat; }
    bool IsMipMap() const { return mBits.mipmap; }
    u16 GetWidth() const { return mWidth; }
    u16 GetHeight() const { return mHeight; }
    s32 GetWrapModeS() const { return mBits.wrapS; }
    s32 GetWrapModeT() const { return mBits.wrapT; }
    s32 GetMinFilter() const { return mBits.minFilter; }
    s32 GetMagFilter() const { return mBits.magFilter; }
    f32 GetMinLOD() const { return mMinLOD; }
    f32 GetMaxLOD() const { return mMaxLOD; }
    f32 GetLODBias() const { return mLODBias / 256.0f; }
    bool IsBiasClampEnable() const { return mBits.biasClampEnable; }
    bool IsEdgeLODEnable() const { return mBits.edgeLODEnable; }
    s32 GetAnisotropy() const { return mBits.anisotropy; }
    s32 GetPaletteFormat() const { return mBits.paletteFormat; }
    u16 GetPaletteEntryNum() const { return mPaletteEntryNum; }

    void SetSize(u16 width, u16 height) {
        mWidth = width;
        mHeight = height;
    }
    void SetTexelFormat(u32 format) { mBits.textureFormat = format; }
    void SetMipMap(bool bEnable) { mBits.mipmap = bEnable; }
    void SetLOD(f32 minLOD, f32 maxLOD) {
        mMinLOD = minLOD;
        mMaxLOD = maxLOD;
    }
    void SetLODBias(f32 bias) { mLODBias = 256.0f * bias; }
    void SetBiasClampEnable(bool bEnable) { mBits.biasClampEnable = bEnable; }
    void SetEdgeLODEnable(bool bEnable) { mBits.edgeLODEnable = bEnable; }
    void SetAnisotropy(u32 aniso) { mBits.anisotropy = aniso; }
    void SetWrapMode(s32 wrapS, s32 wrapT) {
        mBits.wrapS = wrapS;
        mBits.wrapT = wrapT;
    }
    void SetFilter(s32 minFlt, s32 magFlt) {
        mBits.minFilter = minFlt;
        mBits.magFilter = magFlt;
    }

    /* The packed sampling state. */
    struct Bits { /* size: 0x4 */
        u32 textureFormat : 4;   /* +0x0 */
        u32 mipmap : 1;          /* +0x0 */
        u32 wrapS : 2;           /* +0x0 */
        u32 wrapT : 2;           /* +0x0 */
        u32 minFilter : 3;       /* +0x0 */
        u32 magFilter : 1;       /* +0x0 */
        u32 biasClampEnable : 1; /* +0x0 */
        u32 edgeLODEnable : 1;   /* +0x0 */
        u32 anisotropy : 2;      /* +0x0 */
        u32 paletteFormat : 2;   /* +0x0 */
    };

    /* +0x00 */ void* mpImage; /* untyped: the texel bytes */
    /* +0x04 */ void* mpPalette; /* untyped: the TLUT bytes */
    /* +0x08 */ u16 mWidth;
    /* +0x0A */ u16 mHeight;
    /* +0x0C */ f32 mMinLOD;
    /* +0x10 */ f32 mMaxLOD;
    /* +0x14 */ u16 mLODBias;
    /* +0x16 */ u16 mPaletteEntryNum;
    /* +0x18 */ Bits mBits;
};

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_TEXMAP_H */
