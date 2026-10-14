/* homebutton/hkb_animation.cpp - the keyboard layout engine's animation (nw4r::lyt): the step and Hermite curve
 *   evaluators, the basic animation transform (constructor, destructor, resource set, pane/material bind and
 *   animate), the RLAN resource reader, the per-pane animation tree and the animation-link list helpers.
 * RANGE. .text 0x80533854-0x80534D68 (26 functions, in this order: GetStepCurveValue, GetHermiteCurveValue,
 *   AnimTransform::GetFrameSize, AnimTransformBasic's constructor, destructor, SetResource x2, Bind x2, Animate x2,
 *   AnimResource's Init, Set, GetGroupNum, GetGroupArray, IsDescendingBind, GetAnimationShareInfoNum,
 *   GetAnimationShareInfoArray, CalcAnimationNum x2, AnimPaneTree's Init, Set, Bind, detail::FindAnimationLink x2,
 *   detail::UnbindAnimationLink); .data 0x8064A8F8-0x8064A920 (the AnimTransformBasic table); .sdata2
 *   0x8079D5F0-0x8079D610 (-0.001, 0.001, 1, 2, 3, -2, 0, 0.5).
 *   Edges: the left edge is the pool duplicate 1.0 (0x8079D5EC in homebutton/hkb_drawinfo.cpp, 0x8079D5F8 here) and
 *   the `.sdata2` order (0x8079D5F0 is first read by 0x80533854).  The class table .data 0x8064A8F8 holds
 *   0x80533B38..0x805341FC and is stored by 0x80533B04; the next table 0x8064A920 belongs to
 *   homebutton/hkb_resource_accessor.cpp, whose three functions start at 0x80534D68.  The float 0.0 is pooled
 *   again at 0x8079D610 (read by 0x80535454), so the pool ends here.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt for the namespace and every class, member and record name (the layout library's
 *   lyt_animation: the 'RLAN' file test, the 'pai1'/'pat1'/'pah1' blocks, the 'RLPA'/'RLVI'/'RLVC'/'RLMC'/'RLTS'/
 *   'RLTP'/'RLIM' animation kinds, the Hermite constants); the three detail:: rows keep the map's manglings and the
 *   other rows take the compiler's.  Pane, Group and Layout come from homebutton/hkb_layout_decl.h, the two detail::
 *   callees from homebutton/hkb_keyboard.h.
 * RESIDUALS. Bind(Pane*, bool, bool) 0x80533D34 and Bind(Material*, bool) 0x80533EFC: instruction-identical, the
 *   register assignment differs (retail keeps the running link in r31 and the content-offset table in r30, here
 *   they swap; Bind(Material*) shifts this/pMaterial/bDisable/i one register up); declaration order, a separate
 *   local in the inline Bind template, `if (T* p = ...)` and a pointer-then-reference content all measured the same.
 *   AnimPaneTree::Set 0x80534830: the second inlined FindAnimContent holds name/index/offsets in r27/r26/r24 where
 *   retail has r24/r27/r26.
 *   The object also emits a weak AnimTransform table (0x24 B .data) and ~AnimTransform (0x40 B) that no retail
 *   object carries: nothing references them, so the link dead-strips them.
 * SHAPES. `#pragma auto_inline on` is scoped to AnimResource::Init: retail inlines it into Set and nothing else
 *   (unit-wide it also inlines CalcAnimationNum(Pane*) into the Group overload).  TestFileVersion returns the
 *   comparison expression (a bool local spills it to a second register).  The texture SRT and pattern setters are
 *   helpers taking the record's index and component as arguments, so both are read before the Material accessor
 *   call, as in retail; UnbindAnimationLink advances with the postfix iterator (a copy then ++ keeps two copies).
 */
#include "homebutton/hkb_animation.h"

#include "homebutton/hkb_drawinfo.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hkb_layout_decl.h"
#include "homebutton/hkb_material.h"
#include "homebutton/hkb_resource_accessor.h"
#include "homebutton/hkb_texmap.h"
#include "MEM/MEMAllocFromAllocator.h"
#include "OS/OSFastCast.h"

namespace nw4r {
namespace lyt {

namespace detail {

/* Returns whether an RLAN file's version is 0.8 to 0.10. */
inline bool TestFileVersion(const ut::BinaryFileHeader& fileHeader) {
    u8 majorVer = (u8)((fileHeader.version >> 8) & 0xFF);
    u8 minorVer = (u8)(fileHeader.version & 0xFF);
    return majorVer == 0 && 8 <= minorVer && minorVer <= 10;
}
}  // namespace detail

/* Returns whether two frames are within `tolerance` of each other. */
inline bool RIsSame(f32 a, f32 b, f32 tolerance = 1.0e-3f) {
    f32 c = a - b;
    return -tolerance < c && c < tolerance;
}

/* Converts a float to u8 through the paired-single quantiser. */
inline u8 ConvertF32ToU8(f32 value) {
    register f32 arg = value;
    f32 a;
    register f32* ptr = &a;
    register u8 r;
    asm {
        psq_st arg, 0(ptr), 1, OS_FASTCAST_U8
        lbz r, 0(ptr)
    }
    return r;
}

/* 0x80533854 (0xE8): Returns the value of a step curve at `frame`. */
static u16 GetStepCurveValue(f32 frame, const res::StepKey* keyArray, u32 keySize) {
    if (keySize == 1 || frame <= keyArray[0].frame) {
        return keyArray[0].value;
    } else if (frame >= keyArray[keySize - 1].frame) {
        return keyArray[keySize - 1].value;
    }

    int ikeyL = 0;
    int ikeyR = (int)keySize - 1;
    while (ikeyL != ikeyR - 1 && ikeyL != ikeyR) {
        int ikeyCenter = (ikeyL + ikeyR) / 2;
        const res::StepKey& centerKey = keyArray[ikeyCenter];
        if (frame < centerKey.frame) {
            ikeyR = ikeyCenter;
        } else {
            ikeyL = ikeyCenter;
        }
    }

    if (RIsSame(frame, keyArray[ikeyR].frame)) {
        return keyArray[ikeyR].value;
    } else {
        return keyArray[ikeyL].value;
    }
}

/* 0x8053393C (0x1BC): Returns the value of a Hermite curve at `frame`. */
static f32 GetHermiteCurveValue(f32 frame, const res::HermiteKey* keyArray, u32 keySize) {
    if (keySize == 1 || frame <= keyArray[0].frame) {
        return keyArray[0].value;
    } else if (frame >= keyArray[keySize - 1].frame) {
        return keyArray[keySize - 1].value;
    }

    int ikeyL = 0;
    int ikeyR = (int)keySize - 1;
    while (ikeyL != ikeyR - 1 && ikeyL != ikeyR) {
        int ikeyCenter = (ikeyL + ikeyR) / 2;
        if (frame <= keyArray[ikeyCenter].frame) {
            ikeyR = ikeyCenter;
        } else {
            ikeyL = ikeyCenter;
        }
    }

    const res::HermiteKey& key0 = keyArray[ikeyL];
    const res::HermiteKey& key1 = keyArray[ikeyR];

    if (RIsSame(frame, key1.frame)) {
        if (ikeyR < keySize - 1 && key1.frame == keyArray[ikeyR + 1].frame) {
            return keyArray[ikeyR + 1].value;
        } else {
            return key1.value;
        }
    }

    f32 t1 = frame - key0.frame;
    f32 t2 = 1.0f / (key1.frame - key0.frame);
    f32 v0 = key0.value;
    f32 v1 = key1.value;
    f32 s0 = key0.slope;
    f32 s1 = key1.slope;

    f32 t1t1t2 = t1 * t1 * t2;
    f32 t1t1t2t2 = t1t1t2 * t2;
    f32 t1t1t1t2t2 = t1 * t1t1t2t2;
    f32 t1t1t1t2t2t2 = t1t1t1t2t2 * t2;

    return v0 * (2.0f * t1t1t1t2t2t2 - 3.0f * t1t1t2t2 + 1.0f) + v1 * (-2.0f * t1t1t1t2t2t2 + 3.0f * t1t1t2t2) +
           s0 * (t1t1t1t2t2 - 2.0f * t1t1t2 + t1) + s1 * (t1t1t1t2t2 - t1t1t2);
}

/* Sets component `eleIdx` of the material's texture SRT `texSRTIdx`. */
inline void SetTexSRTElement(Material* pMaterial, u32 texSRTIdx, u32 eleIdx, f32 value) {
    f32* srtAry = reinterpret_cast<f32*>(&pMaterial->GetTexSRTAry()[texSRTIdx]);
    srtAry[eleIdx] = value;
}

/* Sets component `eleIdx` of the material's indirect texture SRT `texSRTIdx`. */
inline void SetIndTexSRTElement(Material* pMaterial, u32 texSRTIdx, u32 eleIdx, f32 value) {
    f32* srtAry = reinterpret_cast<f32*>(&pMaterial->GetIndTexSRTAry()[texSRTIdx]);
    srtAry[eleIdx] = value;
}

/* Returns the material's texture map `texMapIdx`. */
inline TexMap& GetTexMap(Material* pMaterial, u32 texMapIdx) {
    return pMaterial->GetTexMapAry()[texMapIdx];
}

/* Applies a pane's translate/rotate/scale/size curves at `frame`. */
inline void AnimatePaneSRT(Pane* pPane, const res::AnimationInfo* pAnimInfo, const u32* animTargetOffsets,
                           f32 frame) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        const res::HermiteKey* keys = detail::ConvertOffsToPtr<res::HermiteKey>(pAnimTarget, pAnimTarget->keysOffset);
        pPane->SetSRTElement(pAnimTarget->target, GetHermiteCurveValue(frame, keys, pAnimTarget->keyNum));
    }
}

/* Applies a pane's visibility curve at `frame`. */
inline void AnimateVisibility(Pane* pPane, const res::AnimationInfo* pAnimInfo, const u32* animTargetOffsets,
                              f32 frame) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        const res::StepKey* keys = detail::ConvertOffsToPtr<res::StepKey>(pAnimTarget, pAnimTarget->keysOffset);
        pPane->SetVisible(GetStepCurveValue(frame, keys, pAnimTarget->keyNum) != 0);
    }
}

/* Applies a pane's vertex colour curves at `frame`. */
inline void AnimateVertexColor(Pane* pPane, const res::AnimationInfo* pAnimInfo, const u32* animTargetOffsets,
                               f32 frame) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        const res::HermiteKey* keys = detail::ConvertOffsToPtr<res::HermiteKey>(pAnimTarget, pAnimTarget->keysOffset);
        f32 value = GetHermiteCurveValue(frame, keys, pAnimTarget->keyNum);
        value += 0.5f;
        u8 u8Val = ConvertF32ToU8(value);
        pPane->SetColorElement(pAnimTarget->target, u8Val);
    }
}

/* Applies a material's colour curves at `frame`, clamped to the TEV colour range. */
inline void AnimateMaterialColor(Material* pMaterial, const res::AnimationInfo* pAnimInfo,
                                 const u32* animTargetOffsets, f32 frame) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        const res::HermiteKey* keys = detail::ConvertOffsToPtr<res::HermiteKey>(pAnimTarget, pAnimTarget->keysOffset);
        f32 value = GetHermiteCurveValue(frame, keys, pAnimTarget->keyNum);
        value += 0.5f;
        s16 s16Val;
        OSf32tos16(&value, &s16Val);
        s16Val = ut::Min<s16>(ut::Max<s16>(s16Val, -1024), 1023);
        pMaterial->SetColorElement(pAnimTarget->target, s16Val);
    }
}

/* Applies a material's texture SRT curves at `frame`. */
inline void AnimateTextureSRT(Material* pMaterial, const res::AnimationInfo* pAnimInfo, const u32* animTargetOffsets,
                              f32 frame) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        if (pAnimTarget->id < pMaterial->mGXMemCap.texSRT) {
            const res::HermiteKey* keys =
                detail::ConvertOffsToPtr<res::HermiteKey>(pAnimTarget, pAnimTarget->keysOffset);
            f32 value = GetHermiteCurveValue(frame, keys, pAnimTarget->keyNum);
            SetTexSRTElement(pMaterial, pAnimTarget->id, pAnimTarget->target, value);
        }
    }
}

/* Applies a material's texture pattern curves at `frame`, swapping in the referenced TPL images. */
/* untyped: caller-owned payload - the 'timg' resources the accessor returned, one TPL file each */
inline void AnimateTexturePattern(Material* pMaterial, const res::AnimationInfo* pAnimInfo,
                                  const u32* animTargetOffsets, f32 frame, void** fileResAry) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        if (pAnimTarget->id < pMaterial->GetTextureNum()) {
            const res::StepKey* keys = detail::ConvertOffsToPtr<res::StepKey>(pAnimTarget, pAnimTarget->keysOffset);
            u16 fileIdx = GetStepCurveValue(frame, keys, pAnimTarget->keyNum);
            if (fileResAry[fileIdx]) {
                GetTexMap(pMaterial, pAnimTarget->id).ReplaceImage((TPLPalette*)fileResAry[fileIdx], 0);
            }
        }
    }
}

/* Applies a material's indirect texture SRT curves at `frame`. */
inline void AnimateIndTexSRT(Material* pMaterial, const res::AnimationInfo* pAnimInfo, const u32* animTargetOffsets,
                             f32 frame) {
    for (int i = 0; i < pAnimInfo->num; ++i) {
        const res::AnimationTarget* pAnimTarget =
            detail::ConvertOffsToPtr<res::AnimationTarget>(pAnimInfo, animTargetOffsets[i]);
        if (pAnimTarget->id < pMaterial->mGXMemCap.indSRT) {
            const res::HermiteKey* keys =
                detail::ConvertOffsToPtr<res::HermiteKey>(pAnimTarget, pAnimTarget->keysOffset);
            f32 value = GetHermiteCurveValue(frame, keys, pAnimTarget->keyNum);
            SetIndTexSRTElement(pMaterial, pAnimTarget->id, pAnimTarget->target, value);
        }
    }
}

/* 0x80533AF8 (0xC): Returns the animation's frame count. */
u16 AnimTransform::GetFrameSize() const {
    return mpRes->frameSize;
}

/* Returns the first animation link from `pLink` on (from the start when NULL) that is bound to nothing. */
inline AnimationLink* AnimTransformBasic::FindUnbindLink(AnimationLink* pLink) const {
    if (pLink == NULL) {
        pLink = mAnimLinkAry;
    }
    for (; pLink < mAnimLinkAry + mAnimLinkNum; ++pLink) {
        if (pLink->GetAnimTransform() == NULL) {
            return pLink;
        }
    }
    return NULL;
}

/* Binds the next free animation link to content `idx` of `pTarget`; returns the link after it, or NULL. */
template <typename T>
inline AnimationLink* AnimTransformBasic::Bind(T* pTarget, AnimationLink* pAnimLink, u16 idx, bool bDisable) {
    pAnimLink = FindUnbindLink(pAnimLink);
    if (!pAnimLink) {
        return NULL;
    }
    pAnimLink->Set(this, idx, bDisable);
    pTarget->AddAnimationLink(pAnimLink);
    return pAnimLink + 1;
}

/* 0x80533B04 (0x34): Constructs a transform with no resource, files or links. */
AnimTransformBasic::AnimTransformBasic() : mpFileResAry(NULL), mAnimLinkAry(NULL), mAnimLinkNum(0) {}

/* 0x80533B38 (0x84): Frees the animation links and the texture file table. */
AnimTransformBasic::~AnimTransformBasic() {
    Layout::DeleteArray(mAnimLinkAry);
    Layout::DeleteArray(mpFileResAry);
}

/* 0x80533BBC (0x14): Sets the resource with one animation link per content. */
void AnimTransformBasic::SetResource(const res::AnimationBlock* pRes, ResourceAccessor* pResAccessor) {
    SetResource(pRes, pResAccessor, pRes->animContNum);
}

/* 0x80533BD0 (0x164): Sets the resource, loads its texture files through `pResAccessor` and allocates `animNum`
 * animation links. */
void AnimTransformBasic::SetResource(const res::AnimationBlock* pRes, ResourceAccessor* pResAccessor,
                                     u16 animNum) {
    mpRes = pRes;
    mpFileResAry = NULL;

    if (mpRes->fileNum > 0) {
        mpFileResAry = Layout::NewArray<void*>(mpRes->fileNum);
        if (mpFileResAry) {
            const u32* fileNameOffsets = detail::ConvertOffsToPtr<u32>(mpRes, sizeof(*mpRes));
            for (int i = 0; i < mpRes->fileNum; ++i) {
                const char* fileName = detail::ConvertOffsToPtr<char>(fileNameOffsets, fileNameOffsets[i]);
                mpFileResAry[i] = pResAccessor->GetResource('timg', fileName, NULL);
            }
        }
    }

    mAnimLinkAry = Layout::NewArray<AnimationLink>(animNum);
    if (mAnimLinkAry) {
        mAnimLinkNum = animNum;
    }
}

/* 0x80533D34 (0x1C8): Binds the contents that name `pPane`'s panes and materials (below it when `bRecursive`). */
void AnimTransformBasic::Bind(Pane* pPane, bool bRecursive, bool bDisable) {
    AnimationLink* pCrAnimLink = NULL;
    const u32* animContOffsets = detail::ConvertOffsToPtr<u32>(mpRes, mpRes->animContOffsetsOffset);

    for (u16 i = 0; i < mpRes->animContNum; ++i) {
        const res::AnimationContent& animCont =
            *detail::ConvertOffsToPtr<res::AnimationContent>(mpRes, animContOffsets[i]);
        if (animCont.type == 0) {
            Pane* pFindPane = pPane->FindPaneByName(animCont.name, bRecursive);
            if (pFindPane) {
                pCrAnimLink = Bind(pFindPane, pCrAnimLink, i, bDisable);
                if (!pCrAnimLink) {
                    break;
                }
            }
        } else {
            Material* pFindMat = pPane->FindMaterialByName(animCont.name, bRecursive);
            if (pFindMat) {
                pCrAnimLink = Bind(pFindMat, pCrAnimLink, i, bDisable);
                if (!pCrAnimLink) {
                    break;
                }
            }
        }
    }
}

/* 0x80533EFC (0x114): Binds the material contents whose name is `pMaterial`'s. */
void AnimTransformBasic::Bind(Material* pMaterial, bool bDisable) {
    AnimationLink* pCrAnimLink = NULL;
    const u32* animContOffsets = detail::ConvertOffsToPtr<u32>(mpRes, mpRes->animContOffsetsOffset);

    for (u16 i = 0; i < mpRes->animContNum; ++i) {
        const res::AnimationContent& animCont =
            *detail::ConvertOffsToPtr<res::AnimationContent>(mpRes, animContOffsets[i]);
        if (animCont.type == 1) {
            if (detail::EqualsMaterialName(pMaterial->mName, animCont.name)) {
                pCrAnimLink = Bind(pMaterial, pCrAnimLink, i, bDisable);
                if (!pCrAnimLink) {
                    break;
                }
            }
        }
    }
}

/* 0x80534010 (0x1EC): Applies content `idx`'s pane animations to `pPane` at the current frame. */
void AnimTransformBasic::Animate(u32 idx, Pane* pPane) {
    const u32* animContOffsets = detail::ConvertOffsToPtr<u32>(mpRes, mpRes->animContOffsetsOffset);
    const res::AnimationContent* pAnimCont =
        detail::ConvertOffsToPtr<res::AnimationContent>(mpRes, animContOffsets[idx]);
    const u32* animInfoOffsets = detail::ConvertOffsToPtr<u32>(pAnimCont, sizeof(*pAnimCont));

    for (int i = 0; i < pAnimCont->num; ++i) {
        const res::AnimationInfo* pAnimInfo =
            detail::ConvertOffsToPtr<res::AnimationInfo>(pAnimCont, animInfoOffsets[i]);
        const u32* animTargetOffsets = detail::ConvertOffsToPtr<u32>(pAnimInfo, sizeof(*pAnimInfo));

        switch (pAnimInfo->kind) {
        case 'RLPA':
            AnimatePaneSRT(pPane, pAnimInfo, animTargetOffsets, mFrame);
            break;
        case 'RLVI':
            AnimateVisibility(pPane, pAnimInfo, animTargetOffsets, mFrame);
            break;
        case 'RLVC':
            AnimateVertexColor(pPane, pAnimInfo, animTargetOffsets, mFrame);
            break;
        }
    }
}

/* 0x805341FC (0x2FC): Applies content `idx`'s material animations to `pMaterial` at the current frame. */
void AnimTransformBasic::Animate(u32 idx, Material* pMaterial) {
    const u32* animContOffsets = detail::ConvertOffsToPtr<u32>(mpRes, mpRes->animContOffsetsOffset);
    const res::AnimationContent* pAnimCont =
        detail::ConvertOffsToPtr<res::AnimationContent>(mpRes, animContOffsets[idx]);
    const u32* animInfoOffsets = detail::ConvertOffsToPtr<u32>(pAnimCont, sizeof(*pAnimCont));

    for (int i = 0; i < pAnimCont->num; ++i) {
        const res::AnimationInfo* pAnimInfo =
            detail::ConvertOffsToPtr<res::AnimationInfo>(pAnimCont, animInfoOffsets[i]);
        const u32* animTargetOffsets = detail::ConvertOffsToPtr<u32>(pAnimInfo, sizeof(*pAnimInfo));

        switch (pAnimInfo->kind) {
        case 'RLMC':
            AnimateMaterialColor(pMaterial, pAnimInfo, animTargetOffsets, mFrame);
            break;
        case 'RLTS':
            AnimateTextureSRT(pMaterial, pAnimInfo, animTargetOffsets, mFrame);
            break;
        case 'RLTP':
            if (mpFileResAry) {
                AnimateTexturePattern(pMaterial, pAnimInfo, animTargetOffsets, mFrame, mpFileResAry);
            }
            break;
        case 'RLIM':
            AnimateIndTexSRT(pMaterial, pAnimInfo, animTargetOffsets, mFrame);
            break;
        }
    }
}

#pragma push
#pragma auto_inline on
/* 0x805344F8 (0x18): Forgets the file and its blocks. */
void AnimResource::Init() {
    mpFileHeader = NULL;
    mpResBlock = NULL;
    mpTagBlock = NULL;
    mpShareBlock = NULL;
}
#pragma pop

/* 0x80534510 (0x110): Reads an RLAN file: checks its header and version and finds its blocks. */
/* untyped: byte range - the RLAN file image */
void AnimResource::Set(const void* anmResBuf) {
    Init();

    const ut::BinaryFileHeader* pFileHeader = static_cast<const ut::BinaryFileHeader*>(anmResBuf);
    if (!detail::TestFileHeader(*pFileHeader, 'RLAN')) {
        return;
    }
    if (!detail::TestFileVersion(*pFileHeader)) {
        return;
    }

    mpFileHeader = pFileHeader;
    const ut::BinaryBlockHeader* pDataBlockHead =
        detail::ConvertOffsToPtr<ut::BinaryBlockHeader>(mpFileHeader, mpFileHeader->headerSize);
    for (int i = 0; i < mpFileHeader->dataBlocks; ++i) {
        switch (pDataBlockHead->kind) {
        case 'pat1':
            mpTagBlock = reinterpret_cast<const res::AnimationTagBlock*>(pDataBlockHead);
            break;
        case 'pah1':
            mpShareBlock = reinterpret_cast<const res::AnimationShareBlock*>(pDataBlockHead);
            break;
        case 'pai1':
            mpResBlock = reinterpret_cast<const res::AnimationBlock*>(pDataBlockHead);
            break;
        }
        pDataBlockHead = detail::ConvertOffsToPtr<ut::BinaryBlockHeader>(pDataBlockHead, pDataBlockHead->size);
    }
}

/* 0x80534620 (0x1C): Returns the number of groups the tag block names, or 0 without one. */
u16 AnimResource::GetGroupNum() const {
    if (!mpTagBlock) {
        return 0;
    }
    return mpTagBlock->groupNum;
}

/* 0x8053463C (0x20): Returns the tag block's group references, or NULL without one. */
const res::AnimationGroupRef* AnimResource::GetGroupArray() const {
    if (!mpTagBlock) {
        return NULL;
    }
    return detail::ConvertOffsToPtr<res::AnimationGroupRef>(mpTagBlock, mpTagBlock->groupsOffset);
}

/* 0x8053465C (0x20): Returns whether the tag binds the animation to the groups' descendants too. */
bool AnimResource::IsDescendingBind() const {
    if (!mpTagBlock) {
        return false;
    }
    return mpTagBlock->flag & 1;
}

/* 0x8053467C (0x1C): Returns the number of animation share records, or 0 without a share block. */
u16 AnimResource::GetAnimationShareInfoNum() const {
    if (!mpShareBlock) {
        return 0;
    }
    return mpShareBlock->shareNum;
}

/* 0x80534698 (0x20): Returns the animation share records, or NULL without a share block. */
const res::AnimationShareInfo* AnimResource::GetAnimationShareInfoArray() const {
    if (!mpShareBlock) {
        return NULL;
    }
    return detail::ConvertOffsToPtr<res::AnimationShareInfo>(mpShareBlock, mpShareBlock->animShareInfoOffset);
}

/* 0x805346B8 (0xD8): Returns how many contents name a pane or material found from `pPane`. */
u16 AnimResource::CalcAnimationNum(Pane* pPane, bool bRecursive) const {
    u16 linkNum = 0;
    const u32* animContOffsets = detail::ConvertOffsToPtr<u32>(mpResBlock, mpResBlock->animContOffsetsOffset);

    for (u16 i = 0; i < mpResBlock->animContNum; ++i) {
        const res::AnimationContent& animCont =
            *detail::ConvertOffsToPtr<res::AnimationContent>(mpResBlock, animContOffsets[i]);
        if (animCont.type == 0) {
            if (pPane->FindPaneByName(animCont.name, bRecursive)) {
                ++linkNum;
            }
        } else {
            if (pPane->FindMaterialByName(animCont.name, bRecursive)) {
                ++linkNum;
            }
        }
    }

    return linkNum;
}

/* 0x80534790 (0x68): Returns the sum of CalcAnimationNum over a group's panes. */
u16 AnimResource::CalcAnimationNum(Group* pGroup, bool bRecursive) const {
    u16 linkNum = 0;
    ut::LinkList<PaneLink, 0>* pPaneList = pGroup->GetPaneList();

    for (ut::LinkList<PaneLink, 0>::Iterator it = pPaneList->GetBeginIter(); it != pPaneList->GetEndIter(); ++it) {
        linkNum += CalcAnimationNum(it->mTarget, bRecursive);
    }

    return linkNum;
}

/* 0x805347F8 (0x38): Clears the tree: no pane content, no material contents. */
void AnimPaneTree::Init() {
    mLinkNum = 0;
    mAnimPaneIdx = 0;
    mAnimMatCnt = 0;
    for (u32 i = 0; i < MATERIAL_NUM_MAX; ++i) {
        mAnimMatIdxs[i] = 0;
    }
}

/* Returns the index of the content of type `animContType` named `animContName`, or 0xFFFF. */
inline u16 FindAnimContent(const res::AnimationBlock* pAnimBlock, const char* animContName, u8 animContType) {
    const u32* animContOffsets = detail::ConvertOffsToPtr<u32>(pAnimBlock, pAnimBlock->animContOffsetsOffset);
    for (u16 i = 0; i < pAnimBlock->animContNum; ++i) {
        const res::AnimationContent* pAnimCont =
            detail::ConvertOffsToPtr<res::AnimationContent>(pAnimBlock, animContOffsets[i]);
        if (pAnimCont->type == animContType && detail::EqualsMaterialName(pAnimCont->name, animContName)) {
            return i;
        }
    }
    return 0xFFFF;
}

/* 0x80534830 (0x260): Records the contents of `animRes` that target `pTargetPane` and its materials. */
void AnimPaneTree::Set(Pane* pTargetPane, const AnimResource& animRes) {
    u16 linkNum = 0;
    const res::AnimationBlock* pAnimBlock = animRes.GetResourceBlock();

    u16 animContIdx = FindAnimContent(pAnimBlock, pTargetPane->GetName(), 0);
    if (animContIdx != 0xFFFF) {
        ++linkNum;
    }

    const u8 animMatCnt = pTargetPane->GetMaterialNum();
    u16 animMatIdxs[MATERIAL_NUM_MAX];
    for (u8 i = 0; i < animMatCnt; ++i) {
        animMatIdxs[i] = FindAnimContent(pAnimBlock, pTargetPane->GetMaterial(i)->mName, 1);
        if (animMatIdxs[i] != 0xFFFF) {
            ++linkNum;
        }
    }

    if (linkNum == 0) {
        return;
    }

    mAnimRes = animRes;
    mAnimPaneIdx = animContIdx;
    mAnimMatCnt = animMatCnt;
    for (u8 i = 0; i < animMatCnt; ++i) {
        mAnimMatIdxs[i] = animMatIdxs[i];
    }
    mLinkNum = linkNum;
}

/* 0x80534A90 (0x1E4): Creates a transform on `pLayout` and binds the recorded contents to `pTargetPane`. */
AnimTransform* AnimPaneTree::Bind(Layout* pLayout, Pane* pTargetPane, ResourceAccessor* pResAccessor) const {
    AnimTransformBasic* pAnimTrans = static_cast<AnimTransformBasic*>(pLayout->CreateAnimTransform());
    pAnimTrans->SetResource(mAnimRes.GetResourceBlock(), pResAccessor, mLinkNum);

    AnimationLink* pCrAnimLink = NULL;
    if (mAnimPaneIdx != 0xFFFF) {
        pCrAnimLink = pAnimTrans->Bind(pTargetPane, pCrAnimLink, mAnimPaneIdx, true);
    }

    const u8 animMatCnt = ut::Min(mAnimMatCnt, pTargetPane->GetMaterialNum());
    for (u8 i = 0; i < animMatCnt; ++i) {
        if (mAnimMatIdxs[i] != 0xFFFF) {
            Material* pMaterial = pTargetPane->GetMaterial(i);
            pCrAnimLink = pAnimTrans->Bind(pMaterial, pCrAnimLink, mAnimMatIdxs[i], true);
        }
    }

    return pAnimTrans;
}

namespace detail {

/* 0x80534C74 (0x34): Returns the link of `pAnimList` bound to `pAnimTrans`, or NULL. */
AnimationLink* FindAnimationLink(AnimationLinkList* pAnimList, AnimTransform* pAnimTrans) {
    for (AnimationLinkList::Iterator it = pAnimList->GetBeginIter(); it != pAnimList->GetEndIter(); ++it) {
        if (pAnimTrans == it->GetAnimTransform()) {
            return &*it;
        }
    }
    return NULL;
}

/* 0x80534CA8 (0x3C): Returns the link of `pAnimList` whose transform plays `animRes`'s animation block, or NULL. */
AnimationLink* FindAnimationLink(AnimationLinkList* pAnimList, const AnimResource& animRes) {
    for (AnimationLinkList::Iterator it = pAnimList->GetBeginIter(); it != pAnimList->GetEndIter(); ++it) {
        if (animRes.GetResourceBlock() == it->GetAnimTransform()->GetAnimResource()) {
            return &*it;
        }
    }
    return NULL;
}

/* 0x80534CE4 (0x84): Unlinks and resets every link bound to `pAnimTrans` (every link when it is NULL). */
void UnbindAnimationLink(AnimationLinkList* pAnimList, AnimTransform* pAnimTrans) {
    AnimationLinkList::Iterator it = pAnimList->GetBeginIter();
    while (it != pAnimList->GetEndIter()) {
        AnimationLinkList::Iterator currIt = it++;
        if (pAnimTrans == NULL || currIt->GetAnimTransform() == pAnimTrans) {
            pAnimList->Erase(currIt.mIterator);
            currIt->Reset();
        }
    }
}

}  // namespace detail

}  // namespace lyt
}  // namespace nw4r
