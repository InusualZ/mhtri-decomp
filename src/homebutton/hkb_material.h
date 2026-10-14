/*
 * homebutton/hkb_material.h - the keyboard layout engine's material (nw4r::lyt::Material) and the GX records it
 *   carries: texture maps, texture SRTs, texture coordinate generators, channel control, TEV swap modes, TEV and
 *   indirect stages, alpha compare and blend mode, plus the binary resource records the constructor reads.
 *   The layouts are read from the constructor, ReserveGXMem and SetupGX in homebutton/hkb_material.cpp.
 */
#ifndef HOMEBUTTON_HKB_MATERIAL_H
#define HOMEBUTTON_HKB_MATERIAL_H

#include "types.h"
#include "gx.h"
#include "nw4r/math.h"
#include "nw4r/fn_805012C4.h"
#include "nw4r/fn_80502828.h"

struct MEMAllocator;
struct TPLPalette;

/* STOPGAP-BEGIN(hbm-material#1) */
/* The scalar placement form of MSL's `<new>`; returns the caller's block unchanged. */
/* untyped: placement allocation hands back the caller-owned byte range */
inline void* operator new(unsigned long size, void* block) {
    (void)size;
    return block;
}
/* STOPGAP-END(hbm-material#1) */

namespace nw4r {
namespace ut {

/* Returns the smaller of two values. */
template <typename T>
inline T Min(T a, T b) {
    return (a > b) ? b : a;
}

/* Returns the larger of two values. */
template <typename T>
inline T Max(T a, T b) {
    return (a < b) ? b : a;
}

/* An intrusive list of T whose LinkListNode sits at byte offset Ofs of each element. size: 0xC */
template <typename T, int Ofs>
class LinkList : public detail::LinkListImpl {
public:
    /* A position in the list, dereferencing to the element. size: 0x4 */
    class Iterator {
    public:
        Iterator(detail::LinkListNode* node) : mIterator(node) {}

        T* operator->() const { return reinterpret_cast<T*>(reinterpret_cast<u8*>(mIterator.mPointer) - Ofs); }
        Iterator& operator++() {
            mIterator.mPointer = mIterator.mPointer->mNext;
            return *this;
        }
        friend bool operator==(Iterator it1, Iterator it2) { return it1.mIterator.mPointer == it2.mIterator.mPointer; }
        friend bool operator!=(Iterator it1, Iterator it2) { return !(it1 == it2); }

        /* +0x0 */ detail::LinkListImpl::Iterator mIterator;
    };

    LinkList() {
        mSize = 0;
        mNode.mNext = &mNode;
        mNode.mPrev = &mNode;
    }

    Iterator GetBeginIter() { return Iterator(mNode.mNext); }
    Iterator GetEndIter() { return Iterator(&mNode); }

    void PushBack(T* p) {
        detail::LinkListImpl::Insert(detail::LinkListImpl::GetEndIter(),
                                     reinterpret_cast<detail::LinkListNode*>(reinterpret_cast<u8*>(p) + Ofs));
    }
};

}  // namespace ut

namespace lyt {

class Material;
class AnimTransform;
class ResourceAccessor;
struct AnimResource;

/* A width and a height in layout units. size: 0x8 */
struct Size {
    Size(f32 aWidth, f32 aHeight) : width(aWidth), height(aHeight) {}
    Size(const Size& other) : width(other.width), height(other.height) {}

    /* +0x0 */ f32 width;
    /* +0x4 */ f32 height;
};

namespace detail {

/* Returns the record `offset` bytes past `baseAddress` in a binary resource. */
/* untyped: byte range - the resource block the offset is relative to */
template <typename T>
inline const T* ConvertOffsToPtr(const void* baseAddress, unsigned int offset) {
    return reinterpret_cast<const T*>(reinterpret_cast<u32>(baseAddress) + offset);
}

}  // namespace detail

namespace res {

/* One texture-name entry of the `txl1` block. size: 0x8 */
struct Texture {
    /* +0x0 */ u32 nameStrOffset;
    /* +0x4 */ u8 type;
    /* +0x5 */ u8 pad_0x5[3];
};

/* The `txl1` block header; the Texture entries follow it. size: 0xC */
struct TextureList {
    /* +0x0 */ ut::BinaryBlockHeader blockHeader;
    /* +0x8 */ u16 texNum;
    /* +0xA */ u8 pad_0xA[2];
};

/* One texture reference of a material resource: the txl1 index and the packed wrap/filter modes. size: 0x4 */
struct TexMap {
    s32 GetWarpModeS() const { return wrapSflt & 3; }
    s32 GetWarpModeT() const { return wrapTflt & 3; }
    s32 GetMinFilter() const { return (((wrapSflt >> 2) & 7) + 1) & 7; }
    s32 GetMagFilter() const { return (((wrapTflt >> 2) & 1) + 1) & 1; }

    /* +0x0 */ u16 texIdx;
    /* +0x2 */ u8 wrapSflt;
    /* +0x3 */ u8 wrapTflt;
};

/* The packed record counts of a material resource. size: 0x4 */
struct MaterialResourceNum {
    u8 GetTexMapNum() const { return bits & 0xF; }
    u8 GetTexSRTNum() const { return (bits >> 4) & 0xF; }
    u8 GetTexCoordGenNum() const { return (bits >> 8) & 0xF; }
    u8 GetChanCtrlNum() const { return (bits >> 25) & 1; }
    u8 GetMatColNum() const { return (bits >> 27) & 1; }
    bool HasTevSwapTable() const { return (bits >> 12) & 1; }
    bool HasAlphaCompare() const { return (bits >> 23) & 1; }
    bool HasBlendMode() const { return (bits >> 24) & 1; }
    u8 GetIndTexSRTNum() const { return (bits >> 13) & 3; }
    u8 GetIndTexStageNum() const { return (bits >> 15) & 7; }
    u8 GetTevStageNum() const { return (bits >> 18) & 0x1F; }

    /* +0x0 */ u32 bits;
};

/* A material resource of the `mat1` block; the variable-length records follow it. size: 0x40 */
struct Material {
    /* +0x00 */ char name[20];
    /* +0x14 */ GXColorS10 tevCols[3];
    /* +0x2C */ ut::Color tevKCols[4];
    /* +0x3C */ MaterialResourceNum resNum;
};

}  // namespace res

/* The resource blocks a layout's materials are built from. size: 0x10 */
struct ResBlockSet {
    /* +0x0 */ const res::TextureList* pTextureList;
    /* +0x4 */ const void* pFontList; /* untyped: the fnl1 block, unread here */
    /* +0x8 */ const void* pMaterialList; /* untyped: the mat1 block, unread here */
    /* +0xC */ ResourceAccessor* pResAccessor;
};

/* STOPGAP-BEGIN(hbm-material#82) */
/* A source of named resources (the layout's archive accessor). size: 0x4 */
class ResourceAccessor {
public:
    virtual ~ResourceAccessor();
    /* untyped: the resource bytes, typed by the caller */
    virtual void* GetResource(u32 resType, const char* name, u32* pSize) = 0;
};
/* STOPGAP-END(hbm-material#82) */

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

    /* STOPGAP-BEGIN(hbm-material#81) */
    void Get(GXTexObj* pTexObj) const;
    void Get(GXTlutObj* pTlutObj) const;
    void ReplaceImage(TPLPalette* pTplRes, u32 id);
    /* STOPGAP-END(hbm-material#81) */

    s32 GetTexelFormat() const { return mBits.textureFormat; }
    u16 GetWidth() const { return mWidth; }
    u16 GetHeight() const { return mHeight; }

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

/* A texture scale/rotate/translate. size: 0x14 */
struct TexSRT {
    /* +0x00 */ math::VEC2 translate;
    /* +0x08 */ f32 rotate;
    /* +0x0C */ math::VEC2 scale;
};

/* One texture coordinate generator. size: 0x4 */
struct TexCoordGen {
    TexCoordGen() : reserve(0) { Set(1, 4, 60); }

    void Set(u8 aTexGenType, u8 aTexGenSrc, u8 aTexMtx) {
        texGenType = aTexGenType;
        texGenSrc = aTexGenSrc;
        texMtx = aTexMtx;
    }

    s32 GetTexGenType() const { return texGenType; }
    s32 GetTexGenSrc() const { return texGenSrc; }
    u32 GetTexMtx() const { return texMtx; }

    /* +0x0 */ u8 texGenType;
    /* +0x1 */ u8 texGenSrc;
    /* +0x2 */ u8 texMtx;
    /* +0x3 */ u8 reserve;
};

/* The colour-channel material sources. size: 0x4 */
struct ChanCtrl {
    ChanCtrl() : matSrcCol(1), matSrcAlp(1), reserve1(0), reserve2(0) {}

    s32 GetColorSrc() const { return matSrcCol; }
    s32 GetAlphaSrc() const { return matSrcAlp; }

    /* +0x0 */ u8 matSrcCol;
    /* +0x1 */ u8 matSrcAlp;
    /* +0x2 */ u8 reserve1;
    /* +0x3 */ u8 reserve2;
};

/* One TEV swap table entry, two bits per channel. size: 0x1 */
struct TevSwapMode {
    u8 GetR() const { return swap & 3; }
    u8 GetG() const { return (swap >> 2) & 3; }
    u8 GetB() const { return (swap >> 4) & 3; }
    u8 GetA() const { return (swap >> 6) & 3; }
    void Set(u8 r, u8 g, u8 b, u8 a) { swap = (u8)(r | (g << 2) | (b << 4) | (a << 6)); }

    /* +0x0 */ u8 swap;
};

/* The alpha test: two comparisons and their combining operation. size: 0x4 */
struct AlphaCompare {
    AlphaCompare() : comp(0x77), op(0), ref0(0), ref1(0) {}

    u8 GetComp0() const { return comp & 0xF; }
    u8 GetComp1() const { return (comp >> 4) & 0xF; }
    u8 GetOp() const { return op; }
    u8 GetRef0() const { return ref0; }
    u8 GetRef1() const { return ref1; }

    /* +0x0 */ u8 comp;
    /* +0x1 */ u8 op;
    /* +0x2 */ u8 ref0;
    /* +0x3 */ u8 ref1;
};

/* The framebuffer blend equation. size: 0x4 */
struct BlendMode {
    BlendMode() : type(1), srcFactor(4), dstFactor(5), op(15) {}

    u8 GetType() const { return type; }
    u8 GetSrcFactor() const { return srcFactor; }
    u8 GetDstFactor() const { return dstFactor; }
    u8 GetOp() const { return op; }

    /* +0x0 */ u8 type;
    /* +0x1 */ u8 srcFactor;
    /* +0x2 */ u8 dstFactor;
    /* +0x3 */ u8 op;
};

/* One indirect texture stage. size: 0x4 */
struct IndirectStage {
    IndirectStage() : texCoordGen(0), texMap(0), scaleS(0), scaleT(0) {}

    u8 GetTexCoordGen() const { return texCoordGen; }
    u8 GetTexMap() const { return texMap; }
    u8 GetScaleS() const { return scaleS; }
    u8 GetScaleT() const { return scaleT; }

    /* +0x0 */ u8 texCoordGen;
    /* +0x1 */ u8 texMap;
    /* +0x2 */ u8 scaleS;
    /* +0x3 */ u8 scaleT;
};

/* One TEV stage: order, swap, colour and alpha combiners and the indirect setup, bit-packed. size: 0x10 */
struct TevStage {
    TevStage() {
        texCoordGen = 0xFF;
        colChan = 4;
        texMap = 0xFF;
        swapSel = 0;
        colorIn[0] = 0xFF;
        colorIn[1] = 0xAF;
        alphaIn[0] = 0x77;
        alphaIn[1] = 0x57;
        colorOp = 0;
        colorOpSel = 0x61;
        alphaOp = 0;
        alphaOpSel = 0x81;
        indStage = 0;
        indBiMt = 0;
        indWrap = 0;
        indFoAdUtAl = 0;
    }

    u8 GetTexCoordGen() const { return texCoordGen; }
    u8 GetColorChan() const { return colChan; }
    u16 GetTexMap() const { return (u16)(texMap | ((swapSel & 1) << 8)); }
    u8 GetRasSwapSel() const { return (swapSel >> 1) & 3; }
    u8 GetTexSwapSel() const { return (swapSel >> 3) & 3; }
    u8 GetColorInA() const { return colorIn[0] & 0xF; }
    u8 GetColorInB() const { return (colorIn[0] >> 4) & 0xF; }
    u8 GetColorInC() const { return colorIn[1] & 0xF; }
    u8 GetColorInD() const { return (colorIn[1] >> 4) & 0xF; }
    u8 GetColorOp() const { return colorOp & 0xF; }
    u8 GetColorBias() const { return (colorOp >> 4) & 3; }
    u8 GetColorScale() const { return (colorOp >> 6) & 3; }
    u8 IsColorClamp() const { return colorOpSel & 1; }
    u8 GetColorOutReg() const { return (colorOpSel >> 1) & 3; }
    u8 GetKColorSel() const { return (colorOpSel >> 3) & 0x1F; }
    u8 GetAlphaInA() const { return alphaIn[0] & 0xF; }
    u8 GetAlphaInB() const { return (alphaIn[0] >> 4) & 0xF; }
    u8 GetAlphaInC() const { return alphaIn[1] & 0xF; }
    u8 GetAlphaInD() const { return (alphaIn[1] >> 4) & 0xF; }
    u8 GetAlphaOp() const { return alphaOp & 0xF; }
    u8 GetAlphaBias() const { return (alphaOp >> 4) & 3; }
    u8 GetAlphaScale() const { return (alphaOp >> 6) & 3; }
    u8 IsAlphaClamp() const { return alphaOpSel & 1; }
    u8 GetAlphaOutReg() const { return (alphaOpSel >> 1) & 3; }
    u8 GetKAlphaSel() const { return (alphaOpSel >> 3) & 0x1F; }
    u8 GetIndStage() const { return indStage; }
    u8 GetIndFormat() const { return indFoAdUtAl & 3; }
    u8 GetIndBiasSel() const { return indBiMt & 7; }
    u8 GetIndMtxSel() const { return (indBiMt >> 3) & 0xF; }
    u8 GetIndWrapS() const { return indWrap & 7; }
    u8 GetIndWrapT() const { return (indWrap >> 3) & 7; }
    u8 IsIndAddPrev() const { return (indFoAdUtAl >> 2) & 1; }
    u8 IsIndUtcLod() const { return (indFoAdUtAl >> 3) & 1; }
    u8 GetIndAlphaSel() const { return (indFoAdUtAl >> 4) & 3; }

    /* +0x0 */ u8 texCoordGen;
    /* +0x1 */ u8 colChan;
    /* +0x2 */ u8 texMap;
    /* +0x3 */ u8 swapSel;
    /* +0x4 */ u8 colorIn[2];
    /* +0x6 */ u8 colorOp;
    /* +0x7 */ u8 colorOpSel;
    /* +0x8 */ u8 alphaIn[2];
    /* +0xA */ u8 alphaOp;
    /* +0xB */ u8 alphaOpSel;
    /* +0xC */ u8 indStage;
    /* +0xD */ u8 indBiMt;
    /* +0xE */ u8 indWrap;
    /* +0xF */ u8 indFoAdUtAl;
};

/* One animation bound to a pane or material: the transform, the target index and the enable flag. size: 0x10 */
class AnimationLink {
public:
    AnimTransform* GetAnimTransform() const { return mAnimTrans; }
    u16 GetIndex() const { return mIdx; }
    bool IsEnable() const { return !mbDisable; }
    void SetEnable(bool bEnable) { mbDisable = !bEnable; }

    /* +0x00 */ ut::detail::LinkListNode mLink;
    /* +0x08 */ AnimTransform* mAnimTrans;
    /* +0x0C */ u16 mIdx;
    /* +0x0E */ bool mbDisable;
};

typedef ut::LinkList<AnimationLink, 0> AnimationLinkList;

/* STOPGAP-BEGIN(hbm-material#83) */
/* An animation that can be bound to panes and materials and applied per frame. size: 0x4 (approximation: the
 * fields past the vtable pointer are not read here) */
class AnimTransform {
public:
    virtual ~AnimTransform();
    virtual void SetResource(const void* pRes, ResourceAccessor* pResAccessor) = 0; /* untyped: the animation block */
    virtual void SetResource(const void* pRes, ResourceAccessor* pResAccessor, u16 animNum) = 0; /* untyped: the animation block */
    virtual void Bind(void* pPane, bool bRecursive, bool bDisable) = 0; /* untyped: the pane, unread here */
    virtual void Bind(Material* pMaterial, bool bDisable) = 0;
    virtual void Animate(u32 idx, void* pPane) = 0; /* untyped: the pane, unread here */
    virtual void Animate(u32 idx, Material* pMaterial) = 0;
};
/* STOPGAP-END(hbm-material#83) */

/* The record counts a material has room for (mGXMemCap) or uses (mGXMemNum), bit-packed. size: 0x4 */
struct BitGXNums {
    u32 texMap : 4;      /* +0x0 */
    u32 texSRT : 4;      /* +0x0 */
    u32 texCoordGen : 4; /* +0x0 */
    u32 indSRT : 2;      /* +0x0 */
    u32 indStage : 3;    /* +0x0 */
    u32 tevSwap : 1;     /* +0x0 */
    u32 tevStage : 5;    /* +0x0 */
    u32 chanCtrl : 1;    /* +0x0 */
    u32 matCol : 1;      /* +0x0 */
    u32 alpComp : 1;     /* +0x0 */
    u32 blendMode : 1;   /* +0x0 */
};

/* A layout material: TEV colours, the GX records in one allocated block, and the bound animations. size: 0x5C */
class Material {
public:
    Material(const res::Material* pRes, const ResBlockSet& resBlockSet);
    virtual ~Material();
    virtual bool SetupGX(bool bModVtxCol, u8 alpha);
    virtual void BindAnimation(AnimTransform* pAnimTrans);
    virtual void UnbindAnimation(AnimTransform* pAnimTrans);
    virtual void UnbindAllAnimation();
    virtual void Animate();
    virtual AnimationLink* FindAnimationLink(AnimTransform* pAnimTrans);
    virtual AnimationLink* FindAnimationLink(const AnimResource& animRes);
    virtual void SetAnimationEnable(AnimTransform* pAnimTrans, bool bEnable);
    virtual void SetAnimationEnable(const AnimResource& animRes, bool bEnable);

    void Init();
    void ReserveGXMem(u8 texMapNum, u8 texSRTNum, u8 texCoordGenNum, u8 tevStageNum, bool allocTevSwap,
                      u8 indStageNum, u8 indSRTNum, bool allocChanCtrl, bool allocMatCol, bool allocAlpComp,
                      bool allocBlendMode);
    const TexMap* GetTexMapAry() const;
    TexMap* GetTexMapAry();
    TexSRT* GetTexSRTAry();
    TexCoordGen* GetTexCoordGenAry();
    ChanCtrl* GetChanCtrlAry();
    ut::Color* GetMatColAry();
    TevSwapMode* GetTevSwapAry();
    AlphaCompare* GetAlphaComparePtr();
    BlendMode* GetBlendModePtr();
    IndirectStage* GetIndirectStageAry();
    TexSRT* GetIndTexSRTAry();
    TevStage* GetTevStageAry();
    void SetName(const char* name);
    void SetTextureNum(u8 num);
    void SetTexCoordGenNum(u8 num);
    void SetTevStageNum(u8 num);
    void SetIndStageNum(u8 num);
    void SetColorElement(u32 colorType, s16 value);
    void AddAnimationLink(AnimationLink* pAnimLink);

    u8 GetTextureNum() const { return mGXMemNum.texMap; }
    bool IsChanCtrlCap() const { return mGXMemCap.chanCtrl; }
    bool IsMatColorCap() const { return mGXMemCap.matCol; }
    bool IsTevSwapCap() const { return mGXMemCap.tevSwap; }
    bool IsAlphaCompareCap() const { return mGXMemCap.alpComp; }
    bool IsBlendModeCap() const { return mGXMemCap.blendMode; }

    /* +0x04 */ AnimationLinkList mAnimList;
    /* +0x10 */ GXColorS10 mTevCols[3];
    /* +0x28 */ ut::Color mTevKCols[4];
    /* +0x38 */ BitGXNums mGXMemCap;
    /* +0x3C */ BitGXNums mGXMemNum;
    /* +0x40 */ void* mpGXMem; /* untyped: the packed GX record block the Get*Ary accessors carve up */
    /* +0x44 */ char mName[21];
    /* +0x59 */ bool mbUserAllocated;
};

namespace detail {

/* Returns the size of the material's texture `texMapIdx`, or 0 x 0 when it has no such texture. */
const Size GetTextureSize(Material* pMaterial, u8 texMapIdx);

}  // namespace detail

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_MATERIAL_H */
