/* homebutton/hkb_material.cpp - the software keyboard layout engine's material (nw4r::lyt::Material): the GX record
 *   block (texture maps, SRTs, coordinate generators, channel control, swap table, TEV and indirect stages, alpha
 *   compare, blend mode), its construction from a `mat1` resource, SetupGX and the animation links.
 * RANGE. .text 0x8053072C-0x80533474 (74 functions); .data 0x8064A878-0x8064A8E8 (kColSels, kAlpSels, the vtable);
 *   .sdata2 0x8079D5B0-0x8079D5D0 (the float pool); .sbss2 0x8079D7F0-0x8079D7F8 (DefaultBlackColor).
 *   Edges: the left edge follows homebutton/hkb_bounding.cpp's static initialiser 0x80530720; the right edge is the
 *   `.sdata2` pool duplicate 4330000000000000 (0x8079D5C8 here, 0x8079D5D8 in homebutton/hkb_texmap.cpp).
 * FLAGS. the homebutton band's cflags_hkb (GC/3.0a5.2, cflags_nw4r with -inline noauto) plus `#pragma auto_inline on`:
 *   retail inlines the in-file members (GetTexMapAry and the other Get*Ary accessors into SetTextureNum, SetupGX,
 *   ReserveGXMem) that -inline noauto leaves as calls.
 * NAMES. GUESS: nw4r::lyt for the namespace and every class, member and static name, from the API shape (the `mat1`
 *   material of NintendoWare's layout library: CalcTextureMtx, SetIndTexMtx, ReserveGXMem, SetupGX, the anim links)
 *   and the nw4r::ut / nw4r::math callees it shares with the game; the map carries these names (the own rows and the
 *   layout callees are renamed), except the SetupGX statics kColSels / kAlpSels, whose spelling is per compiler.
 * SHAPES. the constructor sits in a `#pragma dont_inline on` region: retail calls every getter, template and copy
 *   operator out of line there while the rest of the unit inlines them. SetupGX tests the record block through the
 *   Is*Cap accessors, which lets the compiler reuse one mGXMemCap load for the test and the address.
 * RESIDUALS. Init (0x805313A8): retail copies DefaultBlackColor as one aggregate (four lha, then four sth); src/gx.h
 *   declares GXColorS10::operator= out of line, so the copy here is field by field.
 *   ReserveGXMem (0x805314C4): retail builds each default SRT through nw4r::math::VEC2(0,0) / VEC2(1,1) temporaries
 *   (frame 0x60); src/nw4r/math.h's VEC2 has no constructor, so the fields are stored directly (frame 0x40).
 *   Constructor (0x80530A50): mTevKCols copy through ut::Color::operator=(const Color&) where retail calls
 *   operator=(GXColor) (0x800647A0), and the TEV colour loop indexes its destination where retail walks a pointer.
 *   AnimationLinkList constructor (0x80530A30): retail keeps LinkListNode's two zero stores before Initialize_; the
 *   shared LinkListImpl declares no constructor, so they fold away here.
 *   SetupGX (0x805323D4): the `bUseVtxCol && bModVtxCol` materialisation (retail sets 1, then clears it on !bModVtxCol).
 *   Flip blockers: flipcheck reports .text 0x2D38 against the claimed 0x2D48 (-0x10) and the bytes differ from +0x304
 *   (retail emits the AnimationLinkList constructor before the Material constructor, and the copy operators before
 *   the templates); our object also emits five weak copies the target object lacks (ut::Color ctor, dtor and
 *   operator=(const Color&), math::VEC2 operator=, the LinkList dtor), which retail resolves to other units' copies.
 */
#pragma auto_inline on

#include "homebutton/hkb_material.h"

#include "MSL_C/string.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "nw4r/math_triangular.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"

namespace nw4r {
namespace lyt {

/* STOPGAP-BEGIN(hbm-material#2) */
/* A layout: its pane tree and resources; only the shared allocator is used here. size: 0x1 (approximation: no
 * instance field is read) */
struct Layout {
    /* The allocator every layout object and material record block comes from. */
    static MEMAllocator* mspAllocator; /* 0x80795A20 */
};
/* STOPGAP-END(hbm-material#2) */

namespace detail {
/* STOPGAP-BEGIN(hbm-material#3) */
AnimationLink* FindAnimationLink(AnimationLinkList* pAnimList, AnimTransform* pAnimTrans);
AnimationLink* FindAnimationLink(AnimationLinkList* pAnimList, const AnimResource& animRes);
void UnbindAnimationLink(AnimationLinkList* pAnimList, AnimTransform* pAnimTrans);
/* STOPGAP-END(hbm-material#3) */
/* STOPGAP-BEGIN(hbm-material#4) */
const ut::Color MultipleAlpha(const ut::Color col, u8 alpha);
/* STOPGAP-END(hbm-material#4) */
}  // namespace detail

}  // namespace lyt
}  // namespace nw4r

namespace nw4r {
namespace lyt {

/* The default TEV colour 0 (black); colours 1 and 2 default to white (255, 255, 255, 255). */
static const GXColorS10 DefaultBlackColor = {0, 0, 0, 0};

/* Returns the cosine of an angle in degrees. */
static inline f32 CosDeg(f32 deg) {
    return math::CosFIdx(deg * (256.0f / 360.0f));
}

/* Returns the sine of an angle in degrees. */
static inline f32 SinDeg(f32 deg) {
    return math::SinFIdx(deg * (256.0f / 360.0f));
}

/* Copies a signed TEV colour component by component. */
static inline void CopyColorS10(GXColorS10* pDst, const GXColorS10& src) {
    pDst->r = src.r;
    pDst->g = src.g;
    pDst->b = src.b;
    pDst->a = src.a;
}

/* Sets a signed TEV colour to opaque white. */
static inline void SetWhiteColorS10(GXColorS10* pDst) {
    pDst->r = 255;
    pDst->g = 255;
    pDst->b = 255;
    pDst->a = 255;
}

/* Returns whether two signed TEV colours are equal. */
static inline bool IsSameColor(const GXColorS10& a, const GXColorS10& b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

/* Returns whether a signed TEV colour is opaque white. */
static inline bool IsWhiteColor(const GXColorS10& a) {
    return a.r == 255 && a.g == 255 && a.b == 255 && a.a == 255;
}

static inline void InitBitGXNums(BitGXNums* ptr) {
    ptr->texMap = 0;
    ptr->texSRT = 0;
    ptr->texCoordGen = 0;
    ptr->indSRT = 0;
    ptr->indStage = 0;
    ptr->tevSwap = 0;
    ptr->tevStage = 0;
    ptr->chanCtrl = 0;
    ptr->matCol = 0;
    ptr->alpComp = 0;
    ptr->blendMode = 0;
}

static inline u32 CalcOffsetTexSRTAry(const BitGXNums& bitGXNums) {
    return sizeof(TexMap) * bitGXNums.texMap;
}

static inline u32 CalcOffsetTexCoordGenAry(const BitGXNums& bitGXNums) {
    return CalcOffsetTexSRTAry(bitGXNums) + sizeof(TexSRT) * bitGXNums.texSRT;
}

static inline u32 CalcOffsetChanCtrlAry(const BitGXNums& bitGXNums) {
    return CalcOffsetTexCoordGenAry(bitGXNums) + sizeof(TexCoordGen) * bitGXNums.texCoordGen;
}

static inline u32 CalcOffsetMatColAry(const BitGXNums& bitGXNums) {
    return CalcOffsetChanCtrlAry(bitGXNums) + sizeof(ChanCtrl) * bitGXNums.chanCtrl;
}

static inline u32 CalcOffsetTevSwapAry(const BitGXNums& bitGXNums) {
    return CalcOffsetMatColAry(bitGXNums) + sizeof(ut::Color) * bitGXNums.matCol;
}

static inline u32 CalcOffsetGetAlphaCompare(const BitGXNums& bitGXNums) {
    return CalcOffsetTevSwapAry(bitGXNums) + sizeof(TevSwapMode) * 4 * bitGXNums.tevSwap;
}

static inline u32 CalcOffsetBlendMode(const BitGXNums& bitGXNums) {
    return CalcOffsetGetAlphaCompare(bitGXNums) + sizeof(AlphaCompare) * bitGXNums.alpComp;
}

static inline u32 CalcOffsetIndirectStageAry(const BitGXNums& bitGXNums) {
    return CalcOffsetBlendMode(bitGXNums) + sizeof(BlendMode) * bitGXNums.blendMode;
}

static inline u32 CalcOffsetIndTexSRTAry(const BitGXNums& bitGXNums) {
    return CalcOffsetIndirectStageAry(bitGXNums) + sizeof(IndirectStage) * bitGXNums.indStage;
}

static inline u32 CalcOffsetTevStageAry(const BitGXNums& bitGXNums) {
    return CalcOffsetIndTexSRTAry(bitGXNums) + sizeof(TexSRT) * bitGXNums.indSRT;
}

/* 0x8053072C (0x108): builds the 3x4 texture matrix of a scale/rotate/translate about the texture centre. */
static void CalcTextureMtx(math::MTX34* pMtx, const TexSRT& texSRT) {
    math::VEC2 center;
    center.x = 0.5f;
    center.y = 0.5f;
    f32 cosR = CosDeg(texSRT.rotate);
    f32 sinR = SinDeg(texSRT.rotate);
    f32 a0;
    f32 a1;

    a0 = cosR * texSRT.scale.x;
    a1 = -sinR * texSRT.scale.y;
    pMtx->m[0][0] = a0;
    pMtx->m[0][1] = a1;
    pMtx->m[0][2] = 0.0f;
    pMtx->m[0][3] = texSRT.translate.x + center.x + a0 * -center.x + a1 * -center.y;

    a0 = sinR * texSRT.scale.x;
    a1 = cosR * texSRT.scale.y;
    pMtx->m[1][0] = a0;
    pMtx->m[1][1] = a1;
    pMtx->m[1][2] = 0.0f;
    pMtx->m[1][3] = texSRT.translate.y + center.y + a0 * -center.x + a1 * -center.y;

    pMtx->m[2][0] = 0.0f;
    pMtx->m[2][1] = 0.0f;
    pMtx->m[2][2] = 1.0f;
    pMtx->m[2][3] = 0.0f;
}

/* 0x80530834 (0x1FC): loads a 2x3 indirect matrix, normalising it into GX's range with a power-of-two exponent. */
static void SetIndTexMtx(_GXIndTexMtxID id, const f32 mtx[2][3]) {
    f32 m00 = mtx[0][0];
    f32 m01 = mtx[0][1];
    f32 m02 = mtx[0][2];
    f32 m10 = mtx[1][0];
    f32 m11 = mtx[1][1];
    f32 m12 = mtx[1][2];
    f32 a00 = __fabsf(m00);
    f32 a01 = __fabsf(m01);
    f32 a02 = __fabsf(m02);
    f32 a10 = __fabsf(m10);
    f32 a11 = __fabsf(m11);
    f32 a12 = __fabsf(m12);
    s8 scaleExp = 0;

    if (a00 >= 1.0f || a01 >= 1.0f || a02 >= 1.0f || a10 >= 1.0f || a11 >= 1.0f || a12 >= 1.0f) {
        do {
            if (scaleExp >= 46) {
                break;
            }
            m00 *= 0.5f;
            m01 *= 0.5f;
            m02 *= 0.5f;
            m10 *= 0.5f;
            m11 *= 0.5f;
            m12 *= 0.5f;
            a00 *= 0.5f;
            a01 *= 0.5f;
            a02 *= 0.5f;
            a10 *= 0.5f;
            a11 *= 0.5f;
            a12 *= 0.5f;
            scaleExp++;
        } while (a00 >= 1.0f || a01 >= 1.0f || a02 >= 1.0f || a10 >= 1.0f || a11 >= 1.0f || a12 >= 1.0f);
    } else if (a00 < 0.5f && a01 < 0.5f && a02 < 0.5f && a10 < 0.5f && a11 < 0.5f && a12 < 0.5f) {
        do {
            m00 *= 2.0f;
            m01 *= 2.0f;
            m02 *= 2.0f;
            m10 *= 2.0f;
            m11 *= 2.0f;
            m12 *= 2.0f;
            a00 *= 2.0f;
            a01 *= 2.0f;
            a02 *= 2.0f;
            a10 *= 2.0f;
            a11 *= 2.0f;
            a12 *= 2.0f;
            scaleExp--;
        } while (a00 < 0.5f && a01 < 0.5f && a02 < 0.5f && a10 < 0.5f && a11 < 0.5f && a12 < 0.5f
                 && scaleExp > -17);
    }

    const f32 outMtx[2][3] = {{m00, m01, m02}, {m10, m11, m12}};
    GXSetIndTexMtx(id, outMtx, scaleExp);
}

#pragma push
#pragma dont_inline on
/* 0x80530A50 (0x664): builds the material from its `mat1` resource: colours, the GX record block and textures. */
Material::Material(const res::Material* pRes, const ResBlockSet& resBlockSet) {
    Init();
    SetName(pRes->name);

    for (int i = 0; i < 3; i++) {
        mTevCols[i] = pRes->tevCols[i];
    }
    for (int i = 0; i < 4; i++) {
        mTevKCols[i] = pRes->tevKCols[i];
    }

    u32 resOffs = sizeof(res::Material);
    const res::TexMap* pResTexMap = detail::ConvertOffsToPtr<res::TexMap>(pRes, resOffs);
    resOffs += sizeof(res::TexMap) * pRes->resNum.GetTexMapNum();
    const TexSRT* resTexSRTs = detail::ConvertOffsToPtr<TexSRT>(pRes, resOffs);
    resOffs += sizeof(TexSRT) * pRes->resNum.GetTexSRTNum();
    const TexCoordGen* resTexCoordGens = detail::ConvertOffsToPtr<TexCoordGen>(pRes, resOffs);
    resOffs += sizeof(TexCoordGen) * pRes->resNum.GetTexCoordGenNum();

    const u8 texMapNum = ut::Min<u8>(pRes->resNum.GetTexMapNum(), 8);
    const u8 texSRTNum = ut::Min<u8>(pRes->resNum.GetTexSRTNum(), 10);
    const u8 texCoordGenNum = ut::Min<u8>(pRes->resNum.GetTexCoordGenNum(), 8);
    const bool allocChanCtrl = pRes->resNum.GetChanCtrlNum() != 0;
    const bool allocMatCol = pRes->resNum.GetMatColNum() != 0;
    const bool allocTevSwap = pRes->resNum.HasTevSwapTable();
    const bool allocAlpComp = pRes->resNum.HasAlphaCompare();
    const bool allocBlendMode = pRes->resNum.HasBlendMode();
    const u8 indTexSRTNum = ut::Min<u8>(pRes->resNum.GetIndTexSRTNum(), 3);
    const u8 indStageNum = ut::Min<u8>(pRes->resNum.GetIndTexStageNum(), 4);
    const u8 tevStageNum = ut::Min<u8>(pRes->resNum.GetTevStageNum(), 16);

    ReserveGXMem(texMapNum, texSRTNum, texCoordGenNum, tevStageNum, allocTevSwap, indStageNum, indTexSRTNum,
                 allocChanCtrl, allocMatCol, allocAlpComp, allocBlendMode);

    if (mpGXMem) {
        SetTextureNum(texMapNum);
        if (texMapNum) {
            const res::Texture* textures =
                detail::ConvertOffsToPtr<res::Texture>(resBlockSet.pTextureList, sizeof(res::TextureList));
            TexMap* texMaps = GetTexMapAry();
            u8 di = 0;
            for (u8 si = 0; si < texMapNum; si++) {
                const char* fileName =
                    detail::ConvertOffsToPtr<char>(textures, textures[pResTexMap[si].texIdx].nameStrOffset);
                TPLPalette* pTplRes =
                    (TPLPalette*)resBlockSet.pResAccessor->GetResource('timg', fileName, NULL);
                texMaps[di].ReplaceImage(pTplRes, 0);
                texMaps[di].SetWrapMode(pResTexMap[si].GetWarpModeS(), pResTexMap[si].GetWarpModeT());
                texMaps[di].SetFilter(pResTexMap[si].GetMinFilter(), pResTexMap[si].GetMagFilter());
                di++;
            }
            SetTextureNum(di);
        }

        TexSRT* texSRTs = GetTexSRTAry();
        for (int i = 0; i < texSRTNum; i++) {
            texSRTs[i].translate = resTexSRTs[i].translate;
            texSRTs[i].rotate = resTexSRTs[i].rotate;
            texSRTs[i].scale = resTexSRTs[i].scale;
        }

        TexCoordGen* texCoordGens = GetTexCoordGenAry();
        SetTexCoordGenNum(texCoordGenNum);
        for (u32 i = 0; i < mGXMemNum.texCoordGen; i++) {
            texCoordGens[i] = resTexCoordGens[i];
        }

        if (allocChanCtrl) {
            *GetChanCtrlAry() = *detail::ConvertOffsToPtr<ChanCtrl>(pRes, resOffs);
            resOffs += sizeof(ChanCtrl);
        }

        if (allocMatCol) {
            *GetMatColAry() = *detail::ConvertOffsToPtr<ut::Color>(pRes, resOffs);
            resOffs += sizeof(ut::Color);
        }

        if (allocTevSwap) {
            const TevSwapMode* resTevSwaps = detail::ConvertOffsToPtr<TevSwapMode>(pRes, resOffs);
            TevSwapMode* tevSwaps = GetTevSwapAry();
            for (int i = 0; i < 4; i++) {
                tevSwaps[i] = resTevSwaps[i];
            }
            resOffs += sizeof(TevSwapMode) * 4;
        }

        if (indTexSRTNum) {
            TexSRT* indTexSRTs = GetIndTexSRTAry();
            const TexSRT* resIndTexSRTs = detail::ConvertOffsToPtr<TexSRT>(pRes, resOffs);
            for (int i = 0; i < indTexSRTNum; i++) {
                indTexSRTs[i] = resIndTexSRTs[i];
            }
        }
        resOffs += sizeof(TexSRT) * pRes->resNum.GetIndTexSRTNum();

        if (indStageNum) {
            SetIndStageNum(indStageNum);
            IndirectStage* indStages = GetIndirectStageAry();
            const IndirectStage* resIndStages = detail::ConvertOffsToPtr<IndirectStage>(pRes, resOffs);
            for (int i = 0; i < indStageNum; i++) {
                indStages[i] = resIndStages[i];
            }
        }
        resOffs += sizeof(IndirectStage) * pRes->resNum.GetIndTexStageNum();

        if (tevStageNum) {
            SetTevStageNum(tevStageNum);
            TevStage* tevStages = GetTevStageAry();
            const TevStage* resTevStages = detail::ConvertOffsToPtr<TevStage>(pRes, resOffs);
            for (int i = 0; i < tevStageNum; i++) {
                tevStages[i] = resTevStages[i];
            }
        }
        resOffs += sizeof(TevStage) * pRes->resNum.GetTevStageNum();

        if (allocAlpComp) {
            *GetAlphaComparePtr() = *detail::ConvertOffsToPtr<AlphaCompare>(pRes, resOffs);
            resOffs += sizeof(AlphaCompare);
        }

        if (allocBlendMode) {
            *GetBlendModePtr() = *detail::ConvertOffsToPtr<BlendMode>(pRes, resOffs);
        }
    }
}
#pragma pop

/* 0x805313A8 (0x70): sets the default TEV colours and an empty GX record block. */
void Material::Init() {
    CopyColorS10(&mTevCols[0], DefaultBlackColor);
    SetWhiteColorS10(&mTevCols[1]);
    SetWhiteColorS10(&mTevCols[2]);
    InitBitGXNums(&mGXMemCap);
    InitBitGXNums(&mGXMemNum);
    mbUserAllocated = false;
    mpGXMem = NULL;
}

/* 0x80531418 (0xAC): unbinds every animation and frees the GX record block. */
Material::~Material() {
    UnbindAllAnimation();
    if (mpGXMem) {
        MEMFreeToAllocator(Layout::mspAllocator, mpGXMem);
        mpGXMem = NULL;
    }
}

/* 0x805314C4 (0x76C): grows the GX record block to the given counts and fills the new records with defaults. */
void Material::ReserveGXMem(u8 texMapNum, u8 texSRTNum, u8 texCoordGenNum, u8 tevStageNum, bool allocTevSwap,
                            u8 indStageNum, u8 indSRTNum, bool allocChanCtrl, bool allocMatCol, bool allocAlpComp,
                            bool allocBlendMode) {
    const int tevSwapNum = allocTevSwap ? 1 : 0;
    const int chanCtrlNum = allocChanCtrl ? 1 : 0;
    const int matColNum = allocMatCol ? 1 : 0;
    const int alpCompNum = allocAlpComp ? 1 : 0;
    const int blendModeNum = allocBlendMode ? 1 : 0;

    if (mGXMemCap.texMap < texMapNum || mGXMemCap.texSRT < texSRTNum || mGXMemCap.texCoordGen < texCoordGenNum
        || mGXMemCap.tevStage < tevStageNum || mGXMemCap.tevSwap < tevSwapNum || mGXMemCap.indStage < indStageNum
        || mGXMemCap.indSRT < indSRTNum || mGXMemCap.chanCtrl < chanCtrlNum || mGXMemCap.matCol < matColNum
        || mGXMemCap.alpComp < alpCompNum || mGXMemCap.blendMode < blendModeNum) {
        if (mpGXMem) {
            MEMFreeToAllocator(Layout::mspAllocator, mpGXMem);
            mpGXMem = NULL;
            InitBitGXNums(&mGXMemCap);
            InitBitGXNums(&mGXMemNum);
        }

        mpGXMem = MEMAllocFromAllocator(
            Layout::mspAllocator,
            sizeof(TexMap) * texMapNum + sizeof(TexSRT) * texSRTNum + sizeof(TexCoordGen) * texCoordGenNum
                + sizeof(ChanCtrl) * chanCtrlNum + sizeof(ut::Color) * matColNum
                + sizeof(TevSwapMode) * 4 * tevSwapNum + sizeof(AlphaCompare) * alpCompNum
                + sizeof(BlendMode) * blendModeNum + sizeof(IndirectStage) * indStageNum
                + sizeof(TexSRT) * indSRTNum + sizeof(TevStage) * tevStageNum);

        if (mpGXMem) {
            mGXMemCap.texMap = texMapNum;
            mGXMemCap.texSRT = texSRTNum;
            mGXMemCap.texCoordGen = texCoordGenNum;
            mGXMemCap.indSRT = indSRTNum;
            mGXMemCap.indStage = indStageNum;
            mGXMemCap.tevSwap = tevSwapNum;
            mGXMemCap.tevStage = tevStageNum;
            mGXMemCap.chanCtrl = chanCtrlNum;
            mGXMemCap.matCol = matColNum;
            mGXMemCap.alpComp = alpCompNum;
            mGXMemCap.blendMode = blendModeNum;

            mGXMemNum.texSRT = mGXMemCap.texSRT;
            TexSRT* texSRTs = GetTexSRTAry();
            const u32 texSRTCount = mGXMemNum.texSRT;
            for (u32 i = 0; i < texSRTCount; i++) {
                texSRTs[i].translate.x = 0.0f;
                texSRTs[i].translate.y = 0.0f;
                texSRTs[i].rotate = 0.0f;
                texSRTs[i].scale.x = 1.0f;
                texSRTs[i].scale.y = 1.0f;
            }

            mGXMemNum.indSRT = mGXMemCap.indSRT;
            TexSRT* indTexSRTs = GetIndTexSRTAry();
            const u32 indSRTCount = mGXMemNum.indSRT;
            for (u32 i = 0; i < indSRTCount; i++) {
                indTexSRTs[i].translate.x = 0.0f;
                indTexSRTs[i].translate.y = 0.0f;
                indTexSRTs[i].rotate = 0.0f;
                indTexSRTs[i].scale.x = 1.0f;
                indTexSRTs[i].scale.y = 1.0f;
            }

            mGXMemNum.chanCtrl = mGXMemCap.chanCtrl;
            if (mGXMemNum.chanCtrl) {
                *GetChanCtrlAry() = ChanCtrl();
            }

            mGXMemNum.matCol = mGXMemCap.matCol;
            if (mGXMemNum.matCol) {
                *GetMatColAry() = ut::Color(0xFFFFFFFF);
            }

            mGXMemNum.tevSwap = mGXMemCap.tevSwap;
            if (mGXMemNum.tevSwap) {
                TevSwapMode* tevSwaps = GetTevSwapAry();
                tevSwaps[0].Set(0, 1, 2, 3);
                tevSwaps[1].Set(0, 0, 0, 3);
                tevSwaps[2].Set(1, 1, 1, 3);
                tevSwaps[3].Set(2, 2, 2, 3);
            }

            mGXMemNum.alpComp = mGXMemCap.alpComp;
            if (mGXMemNum.alpComp) {
                *GetAlphaComparePtr() = AlphaCompare();
            }

            mGXMemNum.blendMode = mGXMemCap.blendMode;
            if (mGXMemNum.blendMode) {
                *GetBlendModePtr() = BlendMode();
            }
        }
    }
}

/* 0x80531C30 (0x8): returns the texture maps (read-only). */
const TexMap* Material::GetTexMapAry() const {
    return detail::ConvertOffsToPtr<TexMap>(mpGXMem, 0);
}

/* 0x80531C38 (0x8): returns the texture maps. */
TexMap* Material::GetTexMapAry() {
    return (TexMap*)detail::ConvertOffsToPtr<TexMap>(mpGXMem, 0);
}

/* 0x80531C40 (0x18): returns the texture SRTs. */
TexSRT* Material::GetTexSRTAry() {
    return (TexSRT*)detail::ConvertOffsToPtr<TexSRT>(mpGXMem, CalcOffsetTexSRTAry(mGXMemCap));
}

/* 0x80531C58 (0x24): returns the texture coordinate generators. */
TexCoordGen* Material::GetTexCoordGenAry() {
    return (TexCoordGen*)detail::ConvertOffsToPtr<TexCoordGen>(mpGXMem, CalcOffsetTexCoordGenAry(mGXMemCap));
}

/* 0x80531C7C (0x2C): returns the channel control record. */
ChanCtrl* Material::GetChanCtrlAry() {
    return (ChanCtrl*)detail::ConvertOffsToPtr<ChanCtrl>(mpGXMem, CalcOffsetChanCtrlAry(mGXMemCap));
}

/* 0x80531CA8 (0x34): returns the material colour. */
ut::Color* Material::GetMatColAry() {
    return (ut::Color*)detail::ConvertOffsToPtr<ut::Color>(mpGXMem, CalcOffsetMatColAry(mGXMemCap));
}

/* 0x80531CDC (0x3C): returns the four TEV swap table entries. */
TevSwapMode* Material::GetTevSwapAry() {
    return (TevSwapMode*)detail::ConvertOffsToPtr<TevSwapMode>(mpGXMem, CalcOffsetTevSwapAry(mGXMemCap));
}

/* 0x80531D18 (0x44): returns the alpha compare record. */
AlphaCompare* Material::GetAlphaComparePtr() {
    return (AlphaCompare*)detail::ConvertOffsToPtr<AlphaCompare>(mpGXMem, CalcOffsetGetAlphaCompare(mGXMemCap));
}

/* 0x80531D5C (0x4C): returns the blend mode record. */
BlendMode* Material::GetBlendModePtr() {
    return (BlendMode*)detail::ConvertOffsToPtr<BlendMode>(mpGXMem, CalcOffsetBlendMode(mGXMemCap));
}

/* 0x80531DA8 (0x54): returns the indirect stages. */
IndirectStage* Material::GetIndirectStageAry() {
    return (IndirectStage*)detail::ConvertOffsToPtr<IndirectStage>(mpGXMem, CalcOffsetIndirectStageAry(mGXMemCap));
}

/* 0x80531DFC (0x5C): returns the indirect texture SRTs. */
TexSRT* Material::GetIndTexSRTAry() {
    return (TexSRT*)detail::ConvertOffsToPtr<TexSRT>(mpGXMem, CalcOffsetIndTexSRTAry(mGXMemCap));
}

/* 0x80531E58 (0x64): returns the TEV stages. */
TevStage* Material::GetTevStageAry() {
    return (TevStage*)detail::ConvertOffsToPtr<TevStage>(mpGXMem, CalcOffsetTevStageAry(mGXMemCap));
}

/* 0x80531EBC (0x3C): copies the material name (20 characters at most). */
void Material::SetName(const char* name) {
    strncpy(mName, name, sizeof(mName) - 1);
    mName[sizeof(mName) - 1] = '\0';
}

/* 0x80531EF8 (0x90): sets the texture count, default-constructing the maps it adds. */
void Material::SetTextureNum(u8 num) {
    if (num) {
        TexMap* texMaps = GetTexMapAry();
        for (u32 i = mGXMemNum.texMap; i < num; i++) {
            new (&texMaps[i]) TexMap();
        }
    }
    mGXMemNum.texMap = num;
}

/* 0x80531F88 (0x88): sets the texture coordinate generator count, defaulting the ones it adds. */
void Material::SetTexCoordGenNum(u8 num) {
    if (num) {
        TexCoordGen* texCoordGens = GetTexCoordGenAry();
        for (u32 i = mGXMemNum.texCoordGen; i < num; i++) {
            new (&texCoordGens[i]) TexCoordGen();
        }
    }
    mGXMemNum.texCoordGen = num;
}

/* 0x80532010 (0x120): sets the TEV stage count, defaulting the stages it adds. */
void Material::SetTevStageNum(u8 num) {
    if (num) {
        TevStage* tevStages = GetTevStageAry();
        for (u32 i = mGXMemNum.tevStage; i < num; i++) {
            new (&tevStages[i]) TevStage();
        }
    }
    mGXMemNum.tevStage = num;
}

/* 0x80532130 (0xC4): sets the indirect stage count, defaulting the stages it adds. */
void Material::SetIndStageNum(u8 num) {
    if (num) {
        IndirectStage* indStages = GetIndirectStageAry();
        for (u32 i = mGXMemNum.indStage; i < num; i++) {
            new (&indStages[i]) IndirectStage();
        }
    }
    mGXMemNum.indStage = num;
}

/* 0x805321F4 (0x1E0): sets one component of the material, TEV or konstant colours (animation target order). */
void Material::SetColorElement(u32 colorType, s16 value) {
    switch (colorType) {
    case 0:
    case 1:
    case 2:
    case 3:
        if (mGXMemNum.matCol >= 1) {
            ut::Color* matCols = GetMatColAry();
            u8 u8Val = ut::Min<s16>(ut::Max<s16>(value, 0), 255);
            switch (colorType % 4) {
            case 0:
                matCols->r = u8Val;
                break;
            case 1:
                matCols->g = u8Val;
                break;
            case 2:
                matCols->b = u8Val;
                break;
            case 3:
                matCols->a = u8Val;
                break;
            }
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: {
        const u32 regIdx = (colorType - 4) / 4;
        switch ((colorType - 4) % 4) {
        case 0:
            mTevCols[regIdx].r = value;
            break;
        case 1:
            mTevCols[regIdx].g = value;
            break;
        case 2:
            mTevCols[regIdx].b = value;
            break;
        case 3:
            mTevCols[regIdx].a = value;
            break;
        }
    } break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31: {
        const u32 regIdx = (colorType - 16) / 4;
        u8 u8Val = ut::Min<s16>(ut::Max<s16>(value, 0), 255);
        switch ((colorType - 16) % 4) {
        case 0:
            mTevKCols[regIdx].r = u8Val;
            break;
        case 1:
            mTevKCols[regIdx].g = u8Val;
            break;
        case 2:
            mTevKCols[regIdx].b = u8Val;
            break;
        case 3:
            mTevKCols[regIdx].a = u8Val;
            break;
        }
    } break;
    }
}

/* 0x805323D4 (0xEA0): programs GX with the material; returns whether the vertex colour must still be applied. */
bool Material::SetupGX(bool bModVtxCol, u8 alpha) {
    static u32 kColSels[8] = {0x1F, 0x1B, 0x17, 0x13, 0x1E, 0x1A, 0x16, 0x12};
    static u32 kAlpSels[8] = {0x1F, 0x1B, 0x17, 0x13, 0x1E, 0x1A, 0x16, 0x12};
    bool bUseVtxCol = true;
    bool bUseMatCol = false;

    GXSetNumChans(1);
    if (IsChanCtrlCap()) {
        const ChanCtrl* chanCtrl = GetChanCtrlAry();
        GXSetChanCtrl(0, false, 0, chanCtrl->GetColorSrc(), 0, 0, 2);
        GXSetChanCtrl(2, false, 0, chanCtrl->GetAlphaSrc(), 0, 0, 2);
        bUseVtxCol = chanCtrl->GetColorSrc() == 1 || chanCtrl->GetAlphaSrc() == 1;
        bUseMatCol = chanCtrl->GetColorSrc() == 0 || chanCtrl->GetAlphaSrc() == 0;
    } else {
        GXSetChanCtrl(4, false, 0, 1, 0, 0, 2);
    }

    bool bUseRasStage = bUseVtxCol && bModVtxCol;

    if (bUseMatCol) {
        ut::Color matCol = 0xFFFFFFFF;
        if (IsMatColorCap()) {
            matCol = *GetMatColAry();
        }
        matCol = detail::MultipleAlpha(matCol, alpha);
        GXSetChanMatColor(4, matCol);
        bUseRasStage = bUseRasStage || matCol.ToU32() != 0xFFFFFFFF;
    }

    bool bUseTexMtx[10];
    bool bSetTexMtx = false;
    for (int i = 0; i < 10; i++) {
        bUseTexMtx[i] = false;
    }

    GXSetNumTexGens(mGXMemNum.texCoordGen);
    if (mGXMemNum.texCoordGen) {
        const TexCoordGen* texCoordGens = GetTexCoordGenAry();
        for (u32 i = 0; i < mGXMemNum.texCoordGen; i++) {
            const u32 texMtx = texCoordGens[i].GetTexMtx();
            if (texCoordGens[i].GetTexGenType() == 1 && texMtx != 60) {
                bUseTexMtx[(texMtx - 30) / 3] = true;
                bSetTexMtx = true;
            }
            GXSetTexCoordGen2(i, texCoordGens[i].GetTexGenType(), texCoordGens[i].GetTexGenSrc(), texMtx, false,
                              125);
        }
    }

    if (bSetTexMtx) {
        const TexSRT* texSRTs = GetTexSRTAry();
        for (u8 i = 0; i < mGXMemNum.texSRT; i++) {
            if (bUseTexMtx[i]) {
                math::MTX34 texMtx;
                CalcTextureMtx(&texMtx, texSRTs[i]);
                GXLoadTexMtxImm(texMtx.m, 30 + i * 3, 1);
            }
        }
    }

    if (mGXMemNum.texMap) {
        const TexMap* texMaps = GetTexMapAry();
        u32 tlutName = 0;
        u32 bigTlutName = 16;
        for (u32 i = 0; i < mGXMemNum.texMap; i++) {
            GXTexObj texObj;
            texMaps[i].Get(&texObj);
            const s32 texFmt = texMaps[i].GetTexelFormat();
            if (texFmt == 8 || texFmt == 9 || texFmt == 10) {
                u32 tlut;
                if (texFmt == 10) {
                    tlut = bigTlutName++;
                } else {
                    tlut = tlutName++;
                }
                GXInitTexObjTlut(&texObj, tlut);
                GXTlutObj tlutObj;
                texMaps[i].Get(&tlutObj);
                GXLoadTlut(&tlutObj, tlut);
            }
            GXLoadTexObj(&texObj, i);
        }
    }

    GXSetTevColorS10(1, mTevCols[0]);
    GXSetTevColorS10(2, mTevCols[1]);
    GXSetTevColorS10(3, mTevCols[2]);
    GXSetTevKColor(0, mTevKCols[0]);
    GXSetTevKColor(1, mTevKCols[1]);
    GXSetTevKColor(2, mTevKCols[2]);
    GXSetTevKColor(3, mTevKCols[3]);

    if (IsTevSwapCap()) {
        const TevSwapMode* tevSwaps = GetTevSwapAry();
        for (int i = 0; i < 4; i++) {
            GXSetTevSwapModeTable(i, tevSwaps[i].GetR(), tevSwaps[i].GetG(), tevSwaps[i].GetB(),
                                  tevSwaps[i].GetA());
        }
    } else {
        GXSetTevSwapModeTable(0, 0, 1, 2, 3);
        GXSetTevSwapModeTable(1, 0, 0, 0, 3);
        GXSetTevSwapModeTable(2, 1, 1, 1, 3);
        GXSetTevSwapModeTable(3, 2, 2, 2, 3);
    }

    bool bUseIndMtx[3];
    bool bSetIndMtx = false;
    for (int i = 0; i < 3; i++) {
        bUseIndMtx[i] = false;
    }

    if (mGXMemNum.tevStage) {
        GXSetNumTevStages(mGXMemNum.tevStage);
        const TevStage* tevStages = GetTevStageAry();
        for (u32 i = 0; i < mGXMemNum.tevStage; i++) {
            GXSetTevOrder(i, tevStages[i].GetTexCoordGen(), tevStages[i].GetTexMap(), tevStages[i].GetColorChan());
            GXSetTevSwapMode(i, tevStages[i].GetRasSwapSel(), tevStages[i].GetTexSwapSel());
            GXSetTevColorIn(i, tevStages[i].GetColorInA(), tevStages[i].GetColorInB(), tevStages[i].GetColorInC(),
                            tevStages[i].GetColorInD());
            GXSetTevColorOp(i, tevStages[i].GetColorOp(), tevStages[i].GetColorBias(), tevStages[i].GetColorScale(),
                            tevStages[i].IsColorClamp(), tevStages[i].GetColorOutReg());
            GXSetTevKColorSel(i, tevStages[i].GetKColorSel());
            GXSetTevAlphaIn(i, tevStages[i].GetAlphaInA(), tevStages[i].GetAlphaInB(), tevStages[i].GetAlphaInC(),
                            tevStages[i].GetAlphaInD());
            GXSetTevAlphaOp(i, tevStages[i].GetAlphaOp(), tevStages[i].GetAlphaBias(), tevStages[i].GetAlphaScale(),
                            tevStages[i].IsAlphaClamp(), tevStages[i].GetAlphaOutReg());
            GXSetTevKAlphaSel(i, tevStages[i].GetKAlphaSel());
            const int indMtxSel = tevStages[i].GetIndMtxSel();
            GXSetTevIndirect(i, tevStages[i].GetIndStage(), tevStages[i].GetIndFormat(), tevStages[i].GetIndBiasSel(),
                             indMtxSel, tevStages[i].GetIndWrapS(), tevStages[i].GetIndWrapT(),
                             tevStages[i].IsIndAddPrev(), tevStages[i].IsIndUtcLod(), tevStages[i].GetIndAlphaSel());
            if (1 <= indMtxSel && indMtxSel <= 3) {
                bUseIndMtx[indMtxSel - 1] = true;
                bSetIndMtx = true;
            }
        }
        bUseRasStage = true;
    } else {
        u8 tevStageID = 0;
        if (mGXMemNum.texMap == 0) {
            GXSetTevOrder(0, 255, 255, 4);
            GXSetTevColorIn(0, 15, 4, 10, 15);
            GXSetTevAlphaIn(0, 7, 2, 5, 7);
            bUseRasStage = true;
            tevStageID = 1;
        } else {
            if (mGXMemNum.texMap == 1) {
                GXSetTevOrder(0, 0, 0, 255);
                GXSetTevColorIn(0, 2, 4, 8, 15);
                GXSetTevAlphaIn(0, 1, 2, 4, 7);
                tevStageID = 1;
            } else {
                if (mGXMemNum.texMap == 2) {
                    GXSetTevOrder(0, 0, 0, 255);
                    GXSetTevColorIn(0, 15, 15, 15, 8);
                    GXSetTevAlphaIn(0, 7, 7, 7, 4);
                    GXSetTevOrder(1, 1, 1, 255);
                    GXSetTevColorIn(1, 8, 0, 14, 15);
                    GXSetTevAlphaIn(1, 4, 0, 6, 7);
                    GXSetTevKColorSel(1, kColSels[0]);
                    GXSetTevKAlphaSel(1, kAlpSels[0]);
                    tevStageID = 2;
                } else {
                    for (u32 i = 0; i < mGXMemNum.texMap; i++) {
                        const u32 stage = tevStageID;
                        GXSetTevOrder(stage, i, i, 255);
                        u32 colorD = 0;
                        if (i == 0) {
                            colorD = 15;
                        }
                        u32 alphaD = 0;
                        if (i == 0) {
                            alphaD = 7;
                        }
                        GXSetTevColorIn(stage, 15, 8, 14, colorD);
                        GXSetTevAlphaIn(stage, 7, 4, 6, alphaD);
                        GXSetTevKColorSel(stage, kColSels[i]);
                        GXSetTevKAlphaSel(stage, kAlpSels[i]);
                        tevStageID++;
                    }
                }

                if (!IsSameColor(mTevCols[0], DefaultBlackColor) || !IsWhiteColor(mTevCols[1])) {
                    const u32 stage = tevStageID;
                    GXSetTevOrder(stage, 255, 255, 255);
                    GXSetTevColorIn(stage, 2, 4, 0, 15);
                    GXSetTevAlphaIn(stage, 1, 2, 0, 7);
                    tevStageID++;
                }
            }

            if (bUseRasStage) {
                const u32 stage = tevStageID;
                GXSetTevOrder(stage, 255, 255, 4);
                GXSetTevColorIn(stage, 15, 0, 10, 15);
                GXSetTevAlphaIn(stage, 7, 0, 5, 7);
                tevStageID++;
            }
        }

        for (u8 i = 0; i < tevStageID; i++) {
            const u32 stage = i;
            GXSetTevColorOp(stage, 0, 0, 0, true, 0);
            GXSetTevAlphaOp(stage, 0, 0, 0, true, 0);
            GXSetTevDirect(stage);
            GXSetTevSwapMode(stage, 0, 0);
        }
        GXSetNumTevStages(tevStageID);
    }

    if (bSetIndMtx) {
        const TexSRT* indTexSRTs = GetIndTexSRTAry();
        for (u32 i = 0; i < mGXMemNum.indSRT; i++) {
            if (bUseIndMtx[i]) {
                const TexSRT& srt = indTexSRTs[i];
                f32 cosR = CosDeg(srt.rotate);
                f32 sinR = SinDeg(srt.rotate);
                const f32 indMtx[2][3] = {{cosR * srt.scale.x, -sinR * srt.scale.y, srt.translate.x},
                                          {sinR * srt.scale.x, cosR * srt.scale.y, srt.translate.y}};
                SetIndTexMtx((_GXIndTexMtxID)(i + 1), indMtx);
            }
        }
    }

    GXSetNumIndStages(mGXMemNum.indStage);
    if (mGXMemNum.indStage) {
        const IndirectStage* indStages = GetIndirectStageAry();
        for (u32 i = 0; i < mGXMemNum.indStage; i++) {
            GXSetIndTexOrder(i, indStages[i].GetTexCoordGen(), indStages[i].GetTexMap());
            GXSetIndTexCoordScale(i, indStages[i].GetScaleS(), indStages[i].GetScaleT());
        }
    }

    if (IsAlphaCompareCap()) {
        const AlphaCompare* alpComp = GetAlphaComparePtr();
        GXSetAlphaCompare(alpComp->GetComp0(), alpComp->GetRef0(), alpComp->GetOp(), alpComp->GetComp1(),
                          alpComp->GetRef1());
    } else {
        GXSetAlphaCompare(7, 0, 0, 7, 0);
    }

    if (IsBlendModeCap()) {
        const BlendMode* blendMode = GetBlendModePtr();
        GXSetBlendMode(blendMode->GetType(), blendMode->GetSrcFactor(), blendMode->GetDstFactor(),
                       blendMode->GetOp());
    } else {
        GXSetBlendMode(1, 4, 5, 15);
    }

    return bUseRasStage && bUseVtxCol;
}

/* 0x80533274 (0x20): binds an animation to this material. */
void Material::BindAnimation(AnimTransform* pAnimTrans) {
    pAnimTrans->Bind(this, false);
}

/* 0x80533294 (0x8): removes an animation's link. */
void Material::UnbindAnimation(AnimTransform* pAnimTrans) {
    detail::UnbindAnimationLink(&mAnimList, pAnimTrans);
}

/* 0x8053329C (0x14): removes every animation link. */
void Material::UnbindAllAnimation() {
    UnbindAnimation(NULL);
}

/* 0x805332B0 (0x78): applies every enabled animation. */
void Material::Animate() {
    for (AnimationLinkList::Iterator it = mAnimList.GetBeginIter(); it != mAnimList.GetEndIter(); ++it) {
        if (it->IsEnable()) {
            AnimTransform* animTrans = it->GetAnimTransform();
            animTrans->Animate(it->GetIndex(), this);
        }
    }
}

/* 0x80533328 (0x34): appends an animation link. */
void Material::AddAnimationLink(AnimationLink* pAnimLink) {
    mAnimList.PushBack(pAnimLink);
}

/* 0x8053335C (0x8): returns the link of an animation transform, or NULL. */
AnimationLink* Material::FindAnimationLink(AnimTransform* pAnimTrans) {
    return detail::FindAnimationLink(&mAnimList, pAnimTrans);
}

/* 0x80533364 (0x8): returns the link of an animation resource, or NULL. */
AnimationLink* Material::FindAnimationLink(const AnimResource& animRes) {
    return detail::FindAnimationLink(&mAnimList, animRes);
}

/* 0x8053336C (0x4C): enables or disables an animation transform's link. */
void Material::SetAnimationEnable(AnimTransform* pAnimTrans, bool bEnable) {
    AnimationLink* pAnimLink = FindAnimationLink(pAnimTrans);
    if (pAnimLink) {
        pAnimLink->SetEnable(bEnable);
    }
}

/* 0x805333B8 (0x4C): enables or disables an animation resource's link. */
void Material::SetAnimationEnable(const AnimResource& animRes, bool bEnable) {
    AnimationLink* pAnimLink = FindAnimationLink(animRes);
    if (pAnimLink) {
        pAnimLink->SetEnable(bEnable);
    }
}

namespace detail {

/* 0x80533404 (0x70): returns the size of the material's texture `texMapIdx`, or 0 x 0 when there is none. */
const Size GetTextureSize(Material* pMaterial, u8 texMapIdx) {
    if (texMapIdx >= pMaterial->GetTextureNum()) {
        return Size(0.0f, 0.0f);
    }
    const TexMap& texMap = pMaterial->GetTexMapAry()[texMapIdx];
    return Size(texMap.GetWidth(), texMap.GetHeight());
}

}  // namespace detail

}  // namespace lyt
}  // namespace nw4r
