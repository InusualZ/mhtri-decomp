/* homebutton/hkb_pane.cpp - the software keyboard layout engine's pane base class (nw4r::lyt::Pane: the pane tree,
 *   its transform, alpha and animations).
 * RANGE. .text 0x8052A13C-0x8052B004 (34 functions); .ctors 0x8056F3E0-0x8056F3E4; .data 0x8064A570-0x8064A5E8;
 *   .sbss 0x80795A18-0x80795A20; .sdata2 0x8079D550-0x8079D570.
 *   Edges: the left edge is KBD/kbd_region_us.c (the KBD library's KBDInitRegionUS and its tables, .data
 *   0x806498C8-0x8064A570); the right edge is this unit's static initialiser 0x8052AFF8 (.ctors word 0x8056F3E0),
 *   followed by homebutton/hkb_layout.cpp, whose `.sbss` claim holds Layout::mspAllocator (0x80795A20).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. Pane's members are the map's manglings (nw4r::lyt, after homebutton/hkb_material.cpp). GUESS:
 *   nw4r::lyt::detail::CalcPaneRect (0x8052A3FC), the pane's rectangle from its base point and size, and
 *   Pane::GetPaneRect (0x8052A484, `b` into it; hbm_text_panel.cpp, hbm_kb_widget.cpp, hbm_kb_cursor.cpp and
 *   tiHKBManager.cpp call it with a DrawInfo); the file name `hkb_pane.cpp` is a GUESS (retail lyt_pane).
 * RESIDUALS. Pane::Pane: retail keeps LinkListNode's zero stores for both lists' sentinel nodes before the list
 *   initialisation (LinkListNode deriving from ut::NonCopyable in src/nw4r/fn_805012C4.h, request hkb-b#4).
 * RESIDUALS. Pane::~Pane: retail reads the child's mbUserAllocated through the node (0xD2) and converts the node with
 *   `subic.`; the LinkList iterator of homebutton/hkb_material.h converts first (request hkb-b#4).
 * RESIDUALS. Pane::CalculateMtx: retail reuses the first mFlag load for the location-adjust test; ours reloads it.
 * RESIDUALS. CalcPaneRect: GetVtxPos's return copy and basePt swap stack slots (0x8 / 0x10), as in
 *   homebutton/hkb_window.cpp and hkb_textbox.cpp.
 * RESIDUALS. our object emits weak LinkList<Pane, 4> / LinkList<AnimationLink, 0> destructors (from the constructor)
 *   the target lacks.
 * RESIDUALS. flipcheck: the class table, Pane::typeInfo and the `.sdata2` pool end in the retail object's 8-byte fill
 *   (0x78 vs 0x74, 0x8 vs 0x4, 0x20 vs 0x1C), beside the four .text rows above.
 * SHAPES. CalcPaneRect copies GetVtxPos's result into basePt (retail's math::VEC2 copy), which keeps the dead copy
 *   retail stores at the end; mGlbAlpha reads the draw alpha into a local before the multiply; the child loop of the
 *   destructor copies the iterator with `it++`.
 */
#include "homebutton/hkb_layout.h"

#include "MSL_C/string.h"
#include "MTX/mtx.h"
#include "RVLGX/GXTransform.h"

namespace nw4r {
namespace lyt {

typedef ut::LinkList<Pane, 4> PaneList;

const ut::detail::RuntimeTypeInfo Pane::typeInfo(NULL);

/* 0x8052A13C (0x138): Builds a pane from its `pan1` block: name, user data, transform, size, alpha and flags. */
Pane::Pane(const res::Pane* pBlock) : mSize(0.0f, 0.0f) {
    mpParent = NULL;
    mpMaterial = NULL;
    mbUserAllocated = false;
    mpUserData = NULL;

    mBasePosition = pBlock->basePosition;

    strncpy(mName, pBlock->name, sizeof(mName) - 1);
    mName[sizeof(mName) - 1] = '\0';
    strncpy(mUserData, pBlock->userData, sizeof(mUserData) - 1);
    mUserData[sizeof(mUserData) - 1] = '\0';

    mTranslate = pBlock->translate;
    mRotate = pBlock->rotate;
    mScale = pBlock->scale;
    mSize = pBlock->size;
    mAlpha = pBlock->alpha;
    mGlbAlpha = mAlpha;
    mFlag = pBlock->flag;
}

/* 0x8052A274 (0x138): Destroys the child panes the layout allocated, the animation links and the material. */
Pane::~Pane() {
    for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter();) {
        PaneList::Iterator currIt = it++;
        mChildList.Erase(currIt.mIterator);
        if (!currIt->IsUserAllocated()) {
            Layout::DeleteObj(currIt.operator->());
        }
    }

    UnbindAnimationSelf(NULL);

    if (mpMaterial && !mpMaterial->mbUserAllocated) {
        Layout::DeleteObj(mpMaterial);
    }
}

/* 0x8052A3AC (0x50): Adds a pane at the end of the child list and makes this pane its parent. */
void Pane::AppendChild(Pane* pChild) {
    mChildList.PushBack(pChild);
    pChild->mpParent = this;
}

namespace detail {

/* 0x8052A3FC (0x88): Returns the pane's rectangle: its base point, extended right by the width and down by the
 * height. */
const ut::Rect CalcPaneRect(const Pane* pPane) {
    ut::Rect ret;
    const math::VEC2 vtxPos = pPane->GetVtxPos();
    math::VEC2 basePt = vtxPos;

    ret.left = basePt.x;
    ret.top = basePt.y;
    ret.right = ret.left + pPane->mSize.width;
    ret.bottom = ret.top - pPane->mSize.height;
    return ret;
}

}  // namespace detail

/* 0x8052A484 (0x4): Returns the pane's rectangle; the draw settings do not change it. */
const ut::Rect Pane::GetPaneRect(const DrawInfo& drawInfo) const {
    return detail::CalcPaneRect(this);
}

/* 0x8052A488 (0xC): Returns white: a plain pane has no vertex colours. */
ut::Color Pane::GetVtxColor(u32 idx) const {
    return ut::Color();
}

/* 0x8052A494 (0x4): Ignores the colour: a plain pane has no vertex colours. */
void Pane::SetVtxColor(u32 idx, ut::Color value) {}

/* 0x8052A498 (0x28): Returns the pane alpha for index 16, else the vertex colour element. */
u8 Pane::GetColorElement(u32 idx) const {
    switch (idx) {
    case 16:
        return mAlpha;
    default:
        return GetVtxColorElement(idx);
    }
}

/* 0x8052A4C0 (0x28): Sets the pane alpha for index 16, else the vertex colour element. */
void Pane::SetColorElement(u32 idx, u8 value) {
    switch (idx) {
    case 16:
        mAlpha = value;
        break;
    default:
        SetVtxColorElement(idx, value);
        break;
    }
}

/* 0x8052A4E8 (0x8): Returns 255: a plain pane has no vertex colours. */
u8 Pane::GetVtxColorElement(u32 idx) const {
    return 255;
}

/* 0x8052A4F0 (0x4): Ignores the element: a plain pane has no vertex colours. */
void Pane::SetVtxColorElement(u32 idx, u8 value) {}

/* 0x8052A4F4 (0xA8): Returns this pane or, when recursing, the first descendant named `findName`, or NULL. */
Pane* Pane::FindPaneByName(const char* findName, bool bRecursive) {
    if (detail::EqualsResName(mName, findName)) {
        return this;
    }

    if (bRecursive) {
        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            if (Pane* pPane = it->FindPaneByName(findName, bRecursive)) {
                return pPane;
            }
        }
    }
    return NULL;
}

/* 0x8052A59C (0xB4): Returns the pane's material or, when recursing, the first descendant material named
 * `findName`, or NULL. */
Material* Pane::FindMaterialByName(const char* findName, bool bRecursive) {
    if (mpMaterial && detail::EqualsMaterialName(mpMaterial->mName, findName)) {
        return mpMaterial;
    }

    if (bRecursive) {
        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            if (Material* pMaterial = it->FindMaterialByName(findName, bRecursive)) {
                return pMaterial;
            }
        }
    }
    return NULL;
}

/* 0x8052A650 (0x2F8): Builds the local and global matrices and the global alpha, then recurses into the children
 * with the alpha the pane passes down. */
void Pane::CalculateMtx(const DrawInfo& drawInfo) {
    if (!IsVisible() && !drawInfo.mFlag.invisiblePaneCalculateMtx) {
        return;
    }

    {
        math::MTX34 mtx1;
        math::MTX34 mtx2;
        math::MTX34 rotateMtx;

        {
            math::VEC2 scale = mScale;
            if (drawInfo.mFlag.locationAdjust && (mFlag & 4)) {
                scale.x *= drawInfo.mLocationAdjustScale.x;
                scale.y *= drawInfo.mLocationAdjustScale.y;
            }
            PSMTXScale(mtx2.m, scale.x, scale.y, 1.0f);
        }

        PSMTXRotRad(rotateMtx.m, 'x', 0.017453292f * mRotate.x);
        PSMTXConcat(rotateMtx.m, mtx2.m, mtx1.m);
        PSMTXRotRad(rotateMtx.m, 'y', 0.017453292f * mRotate.y);
        PSMTXConcat(rotateMtx.m, mtx1.m, mtx2.m);
        PSMTXRotRad(rotateMtx.m, 'z', 0.017453292f * mRotate.z);
        PSMTXConcat(rotateMtx.m, mtx2.m, mtx1.m);

        PSMTXTransApply(mtx1.m, mMtx.m, mTranslate.x, mTranslate.y, mTranslate.z);
    }

    if (mpParent) {
        PSMTXConcat(mpParent->mGlbMtx.m, mMtx.m, mGlbMtx.m);
    } else if (drawInfo.mFlag.mulViewDraw) {
        mGlbMtx = mMtx;
    } else {
        PSMTXConcat(drawInfo.mViewMtx.m, mMtx.m, mGlbMtx.m);
    }

    if (drawInfo.mFlag.receiveAlpha && mpParent) {
        f32 glbAlpha = drawInfo.mGlobalAlpha;
        mGlbAlpha = (u8)(mAlpha * glbAlpha);
    } else {
        mGlbAlpha = mAlpha;
    }

    f32 crGlobalAlpha = drawInfo.mGlobalAlpha;
    bool bCrInfluenced = drawInfo.mFlag.receiveAlpha;
    bool bModDrawInfo = (mFlag & 2) && mAlpha != 255;

    if (bModDrawInfo) {
        DrawInfo& mtDrawInfo = const_cast<DrawInfo&>(drawInfo);
        mtDrawInfo.mGlobalAlpha = (1.0f / 255.0f) * (crGlobalAlpha * mAlpha);
        mtDrawInfo.mFlag.receiveAlpha = true;
    }

    for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
        it->CalculateMtx(drawInfo);
    }

    if (bModDrawInfo) {
        DrawInfo& mtDrawInfo = const_cast<DrawInfo&>(drawInfo);
        mtDrawInfo.mGlobalAlpha = crGlobalAlpha;
        mtDrawInfo.mFlag.receiveAlpha = bCrInfluenced;
    }
}

/* 0x8052A948 (0x84): Draws a visible pane, then its children. */
void Pane::Draw(const DrawInfo& drawInfo) {
    if (IsVisible()) {
        DrawSelf(drawInfo);

        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            it->Draw(drawInfo);
        }
    }
}

/* 0x8052A9CC (0x4): Draws nothing: a plain pane only groups its children. */
void Pane::DrawSelf(const DrawInfo& drawInfo) {}

/* 0x8052A9D0 (0x8C): Applies the pane's animations, then the children's unless an invisible pane is skipped. */
void Pane::Animate(u32 option) {
    AnimateSelf(option);

    if (IsVisible() || !(option & 1)) {
        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            it->Animate(option);
        }
    }
}

/* 0x8052AA5C (0xB4): Applies each enabled animation link to the pane, then animates the material. */
void Pane::AnimateSelf(u32 option) {
    for (AnimationLinkList::Iterator it = mAnimList.GetBeginIter(); it != mAnimList.GetEndIter(); ++it) {
        if (it->IsEnable()) {
            AnimTransform* pAnimTrans = it->GetAnimTransform();
            pAnimTrans->Animate(it->GetIndex(), this);
        }
    }

    if (IsVisible() || !(option & 1)) {
        if (mpMaterial) {
            mpMaterial->Animate();
        }
    }
}

/* 0x8052AB10 (0x1C): Binds an animation to the pane (and its subtree when recursing). */
void Pane::BindAnimation(AnimTransform* pAnimTrans, bool bRecursive, bool bDisable) {
    pAnimTrans->Bind(this, bRecursive, bDisable);
}

/* 0x8052AB2C (0x90): Unbinds an animation from the pane and, when recursing, from its subtree. */
void Pane::UnbindAnimation(AnimTransform* pAnimTrans, bool bRecursive) {
    UnbindAnimationSelf(pAnimTrans);

    if (bRecursive) {
        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            it->UnbindAnimation(pAnimTrans, bRecursive);
        }
    }
}

/* 0x8052ABBC (0x18): Unbinds every animation from the pane and, when recursing, from its subtree. */
void Pane::UnbindAllAnimation(bool bRecursive) {
    UnbindAnimation(NULL, bRecursive);
}

/* 0x8052ABD4 (0x60): Unbinds an animation (every one for NULL) from the material and the pane's own links. */
void Pane::UnbindAnimationSelf(AnimTransform* pAnimTrans) {
    if (mpMaterial) {
        mpMaterial->UnbindAnimation(pAnimTrans);
    }

    detail::UnbindAnimationLink(&mAnimList, pAnimTrans);
}

/* 0x8052AC34 (0x34): Adds an animation link at the end of the pane's list. */
void Pane::AddAnimationLink(AnimationLink* pAnimationLink) {
    mAnimList.PushBack(pAnimationLink);
}

/* 0x8052AC68 (0x8): Returns the pane's link to an animation transform, or NULL. */
AnimationLink* Pane::FindAnimationLinkSelf(AnimTransform* pAnimTrans) {
    return detail::FindAnimationLink(&mAnimList, pAnimTrans);
}

/* 0x8052AC70 (0x8): Returns the pane's link to an animation resource, or NULL. */
AnimationLink* Pane::FindAnimationLinkSelf(const AnimResource& animRes) {
    return detail::FindAnimationLink(&mAnimList, animRes);
}

/* 0x8052AC78 (0x100): Enables or disables an animation transform on the pane, its materials and, when recursing,
 * its subtree. */
void Pane::SetAnimationEnable(AnimTransform* pAnimTrans, bool bEnable, bool bRecursive) {
    if (AnimationLink* pAnimLink = FindAnimationLinkSelf(pAnimTrans)) {
        pAnimLink->SetEnable(bEnable);
    }

    u8 materialNum = GetMaterialNum();
    for (u8 idx = 0; idx < materialNum; ++idx) {
        GetMaterial(idx)->SetAnimationEnable(pAnimTrans, bEnable);
    }

    if (bRecursive) {
        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            it->SetAnimationEnable(pAnimTrans, bEnable, bRecursive);
        }
    }
}

/* 0x8052AD78 (0x100): Enables or disables an animation resource on the pane, its materials and, when recursing,
 * its subtree. */
void Pane::SetAnimationEnable(const AnimResource& animRes, bool bEnable, bool bRecursive) {
    if (AnimationLink* pAnimLink = FindAnimationLinkSelf(animRes)) {
        pAnimLink->SetEnable(bEnable);
    }

    u8 materialNum = GetMaterialNum();
    for (u8 idx = 0; idx < materialNum; ++idx) {
        GetMaterial(idx)->SetAnimationEnable(animRes, bEnable);
    }

    if (bRecursive) {
        for (PaneList::Iterator it = mChildList.GetBeginIter(); it != mChildList.GetEndIter(); ++it) {
            it->SetAnimationEnable(animRes, bEnable, bRecursive);
        }
    }
}

/* 0x8052AE78 (0x58): Loads the global matrix (times the view matrix when the view is applied per draw) as the
 * position matrix. */
void Pane::LoadMtx(const DrawInfo& drawInfo) {
    math::MTX34 mtx;
    const f32(*mtxPtr)[4];

    if (drawInfo.mFlag.mulViewDraw) {
        PSMTXConcat(drawInfo.mViewMtx.m, mGlbMtx.m, mtx.m);
        mtxPtr = mtx.m;
    } else {
        mtxPtr = mGlbMtx.m;
    }

    GXLoadPosMtxImm(mtxPtr, 0);
    GXSetCurrentMtx(0);
}

/* 0x8052AED0 (0xE4): Returns the pane's top-left corner relative to its origin, from the base position. */
const math::VEC2 Pane::GetVtxPos() const {
    math::VEC2 basePt;
    basePt.x = 0.0f;
    basePt.y = 0.0f;

    switch (mBasePosition % 3) {
    default:
    case 0:
        basePt.x = 0.0f;
        break;
    case 1:
        basePt.x = -mSize.width / 2;
        break;
    case 2:
        basePt.x = -mSize.width;
        break;
    }

    switch (mBasePosition / 3) {
    default:
    case 0:
        basePt.y = 0.0f;
        break;
    case 1:
        basePt.y = mSize.height / 2;
        break;
    case 2:
        basePt.y = mSize.height;
        break;
    }

    return basePt;
}

/* 0x8052AFB4 (0x14): Returns 1 when the pane has a material, else 0. */
u8 Pane::GetMaterialNum() const {
    return mpMaterial ? 1 : 0;
}

/* 0x8052AFC8 (0x8): Returns the pane's material. */
Material* Pane::GetMaterial() const {
    return mpMaterial;
}

/* 0x8052AFD0 (0x20): Returns material 0 (the pane's own), or NULL for any other index. */
Material* Pane::GetMaterial(u32 idx) const {
    if (idx == 0) {
        return GetMaterial();
    }
    return NULL;
}

/* 0x8052AFF0 (0x8): Returns the class's run-time type record. */
const ut::detail::RuntimeTypeInfo* Pane::GetRuntimeTypeInfo() const {
    return &typeInfo;
}

}  // namespace lyt
}  // namespace nw4r
