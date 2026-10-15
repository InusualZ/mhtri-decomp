/* homebutton/hkb_layout.cpp - the software keyboard layout engine's groups (nw4r::lyt::Group, GroupContainer), the
 *   layout (nw4r::lyt::Layout: it walks an `RLYT` resource's blocks and builds the pane tree, the groups and the
 *   animations) and the picture pane (nw4r::lyt::Picture).
 * RANGE. .text 0x8052B004-0x8052C880 (34 functions); .ctors 0x8056F3E4-0x8056F3E8; .data 0x8064A5E8-0x8064A6B0;
 *   .sbss 0x80795A20-0x80795A30; .sdata2 0x8079D570-0x8079D580.
 *   Edges: the left edge follows homebutton/hkb_pane.cpp's static initialiser 0x8052AFF8 (.ctors word 0x8056F3E0);
 *   the right edge is this unit's static initialiser 0x8052C874 (.ctors word 0x8056F3E4).
 *   The range is three retail objects: the class tables rise (Group 0x8064A5E8, Layout 0x8064A5F8, Picture
 *   0x8064A638 - one object emits its tables in descending order), the `.sdata2` pool holds 0.0 twice (0x8079D570
 *   for the layout, 0x8079D578 for the picture) and Layout::mspAllocator (0x80795A20, this unit's) sits between the
 *   pane's and the picture's run-time type records; the seams are 0x8052B344 (lyt_group | lyt_layout) and 0x8052C330
 *   (lyt_layout | lyt_picture).
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::Group, GroupContainer, Layout and Picture and their members, from the runtime dump
 *   (nw4hbm::lyt::GroupContainer::AppendGroup / FindGroupByName, Layout::UnbindAnimation, SetAnimationEnable,
 *   Animate, BuildPaneObj, Picture::DrawSelf at their addresses: the HOME Button copy of NintendoWare's layout
 *   library, spelt nw4r::lyt in this band after homebutton/hkb_material.cpp). SetTagProcessorImpl,
 *   IsIncludeAnimationGroupRef and SetMaterialTexCoordGen are GUESS names for the file's static helpers.
 * SHAPES. the list destructors copy the iterator with `it++` (a copy then `++it` keeps a second register); the
 *   GroupContainer destructor hands DeleteObj LinkList::GetPointerFromNode's pointer (see homebutton/hkb_pane.cpp).
 * SHAPES. BuildPaneObj casts the block in each case; Picture::Append sets the coordinate generator through a by-value
 *   helper (retail's Material::SetTexCoordGen, which homebutton/hkb_material.h does not declare: request hkb-b#4).
 * RESIDUALS. the range is three objects (seam request hkb-b#3): the class tables, the 0.0 pool word and the
 *   SetTagProcessorImpl position cannot match as one object; retail fills Layout::mspAllocator's `.sbss` slot to 8
 *   bytes (the end of the lyt_layout object), ours packs Picture::typeInfo at +0x4.
 * SHAPES. the Layout / Group / GroupContainer constructors keep LinkListNode's two zero stores before the list
 *   initialisation because LinkListNode derives from ut::NonCopyable (src/nw4r/ut_LinkList.h).
 * RESIDUALS. our object emits weak LinkList<...>, LinkListNode and ut::NonCopyable destructors the target lacks
 *   (unreferenced, dead-stripped by the link).
 * RESIDUALS. SetTagProcessor / SetTagProcessorImpl: the second inlined level walks the child list with `lwzu` where
 *   retail uses `lwz` + `addi`.
 */
#include "homebutton/hkb_layout.h"

#include "homebutton/hkb_bounding.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hkb_window.h"
#include "MSL_C/string.h"

namespace nw4r {
namespace lyt {

typedef ut::LinkList<Pane, 4> PaneList;
typedef ut::LinkList<PaneLink, 0> PaneLinkList;
typedef ut::LinkList<Group, 4> GroupList;
typedef ut::LinkList<AnimTransform, 4> AnimTransformList;

/* 0x8052B004 (0x11C): Builds a group from its `grp1` block: its name, then a link to each named pane found under
 * the root pane. */
Group::Group(const res::Group* pResGroup, Pane* pRootPane) {
    mbUserAllocated = false;

    strncpy(mName, pResGroup->name, sizeof(mName) - 1);
    mName[sizeof(mName) - 1] = '\0';

    const char* paneNameBase = detail::ConvertOffsToPtr<char>(pResGroup, sizeof(res::Group));
    for (int i = 0; i < pResGroup->paneNum; ++i) {
        Pane* pFindPane = pRootPane->FindPaneByName(paneNameBase + i * 16, true);
        if (pFindPane) {
            AppendPane(pFindPane);
        }
    }
}

/* Links a pane into the group. */
inline void Group::AppendPane(Pane* pPane) {
    PaneLink* pPaneLink = Layout::NewObj<PaneLink>();
    if (pPaneLink) {
        pPaneLink->mTarget = pPane;
        mPaneLinkList.PushBack(pPaneLink);
    }
}

/* 0x8052B120 (0xAC): Unlinks and frees the group's pane links. */
Group::~Group() {
    for (PaneLinkList::Iterator it = mPaneLinkList.GetBeginIter(); it != mPaneLinkList.GetEndIter();) {
        PaneLinkList::Iterator currIt = it++;
        mPaneLinkList.Erase(currIt.mIterator);
        Layout::DeleteObj(currIt.operator->());
    }
}

/* 0x8052B1CC (0xC8): Unlinks the groups and destroys the ones the container allocated. */
GroupContainer::~GroupContainer() {
    for (GroupList::Iterator it = mGroupList.GetBeginIter(); it != mGroupList.GetEndIter();) {
        GroupList::Iterator currIt = it++;
        mGroupList.Erase(currIt.mIterator);
        if (!currIt->IsUserAllocated()) {
            Layout::DeleteObj(GroupList::GetPointerFromNode(currIt.mIterator.mPointer));
        }
    }
}

/* 0x8052B294 (0x34): Adds a group at the end of the container. */
void GroupContainer::AppendGroup(Group* pGroup) {
    mGroupList.PushBack(pGroup);
}

/* 0x8052B2C8 (0x7C): Returns the group named `findName`, or NULL. */
Group* GroupContainer::FindGroupByName(const char* findName) {
    for (GroupList::Iterator it = mGroupList.GetBeginIter(); it != mGroupList.GetEndIter(); ++it) {
        if (detail::EqualsResName(it->GetName(), findName)) {
            return it.operator->();
        }
    }
    return NULL;
}

namespace {

/* 0x8052B344 (0x1AC): Gives every text box in the pane's subtree the tag processor. */
inline void SetTagProcessorImpl(Pane* pPane, ut::TagProcessorBase<wchar_t>* pTagProcessor) {
    if (TextBox* pTextBox = ut::DynamicCast<TextBox>(pPane)) {
        pTextBox->mpTagProcessor = pTagProcessor;
    }

    for (PaneList::Iterator it = pPane->mChildList.GetBeginIter(); it != pPane->mChildList.GetEndIter(); ++it) {
        SetTagProcessorImpl(it.operator->(), pTagProcessor);
    }
}

/* 0x8052B4F0 (0xC8): Returns whether a pane, or one of its parents when the binding descends, is in one of the
 * animation's groups. */
bool IsIncludeAnimationGroupRef(GroupContainer* pGroupContainer, const res::AnimationGroupRef* const groupRefs,
                                u16 bindGroupNum, bool bDescendingBind, Pane* pTargetPane) {
    for (u16 grpIdx = 0; grpIdx < bindGroupNum; ++grpIdx) {
        Group* pGroup = pGroupContainer->FindGroupByName(groupRefs[grpIdx].name);
        PaneLinkList& paneList = pGroup->mPaneLinkList;

        for (PaneLinkList::Iterator it = paneList.GetBeginIter(); it != paneList.GetEndIter(); ++it) {
            const Pane* pLinkTarget = it->mTarget;
            if (pLinkTarget == pTargetPane) {
                return true;
            }

            if (bDescendingBind) {
                for (const Pane* pParentPane = pTargetPane->GetParent(); pParentPane;
                     pParentPane = pParentPane->GetParent()) {
                    if (pLinkTarget == pParentPane) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

/* Sets texture coordinate generator `idx` of a material. */
inline void SetMaterialTexCoordGen(Material* pMaterial, u32 idx, TexCoordGen value) {
    pMaterial->GetTexCoordGenAry()[idx] = value;
}

/* Returns whether a binary file's version is one the layout reads (0.8 to 0.10). */
inline bool TestFileVersion(const ut::BinaryFileHeader& fileHeader) {
    u8 majorVer = (u8)((fileHeader.version >> 8) & 0xFF);
    u8 minorVer = (u8)(fileHeader.version & 0xFF);
    bool bValid = false;

    if (majorVer == 0 && minorVer >= 8 && minorVer <= 10) {
        bValid = true;
    }
    return bValid;
}

}  // namespace

MEMAllocator* Layout::mspAllocator;

/* 0x8052B5B8 (0x40): Builds an empty layout. */
Layout::Layout() : mpRootPane(NULL), mpGroupContainer(NULL), mLayoutSize(0.0f, 0.0f) {}

/* 0x8052B5F8 (0x128): Destroys the groups, the pane tree and the animations. */
Layout::~Layout() {
    DeleteObj(mpGroupContainer);

    if (mpRootPane && !mpRootPane->IsUserAllocated()) {
        DeleteObj(mpRootPane);
    }

    for (AnimTransformList::Iterator it = mAnimTransList.GetBeginIter(); it != mAnimTransList.GetEndIter();) {
        AnimTransformList::Iterator currIt = it++;
        mAnimTransList.Erase(currIt.mIterator);
        DeleteObj(currIt.operator->());
    }
}

/* 0x8052B720 (0x340): Builds the layout from an `RLYT` resource: its size, the texture, font and material lists,
 * the pane tree, the user data and the groups. */
/* untyped: byte range - the RLYT resource image */
bool Layout::Build(const void* lytResBuf, ResourceAccessor* pResAcsr) {
    const ut::BinaryFileHeader* pFileHead = static_cast<const ut::BinaryFileHeader*>(lytResBuf);

    if (!detail::TestFileHeader(*pFileHead, 'RLYT')) {
        return false;
    }
    if (!TestFileVersion(*pFileHead)) {
        return false;
    }

    ResBlockSet resBlockSet = {NULL, NULL, NULL, NULL};
    resBlockSet.pResAccessor = pResAcsr;
    Pane* pParentPane = NULL;
    Pane* pLastPane = NULL;
    bool bReadRootGroup = false;
    int groupNestLevel = 0;
    const void* dataPtr = static_cast<const u8*>(lytResBuf) + pFileHead->headerSize; /* untyped: the block */

    for (int i = 0; i < pFileHead->dataBlocks; ++i) {
        const ut::BinaryBlockHeader* pDataBlockHead = static_cast<const ut::BinaryBlockHeader*>(dataPtr);

        switch ((s32)pDataBlockHead->kind) {
        case 'lyt1': {
            const res::Layout* pResLyt = static_cast<const res::Layout*>(dataPtr);
            mLayoutSize = pResLyt->layoutSize;
        } break;
        case 'txl1':
            resBlockSet.pTextureList = static_cast<const res::TextureList*>(dataPtr);
            break;
        case 'fnl1':
            resBlockSet.pFontList = dataPtr;
            break;
        case 'mat1':
            resBlockSet.pMaterialList = dataPtr;
            break;
        case 'pan1':
        case 'pic1':
        case 'txt1':
        case 'wnd1':
        case 'bnd1': {
            Pane* pPane = BuildPaneObj((s32)pDataBlockHead->kind, dataPtr, resBlockSet);
            if (pPane) {
                if (mpRootPane == NULL) {
                    mpRootPane = pPane;
                }
                if (pParentPane) {
                    pParentPane->AppendChild(pPane);
                }
                pLastPane = pPane;
            }
        } break;
        case 'usd1':
            pLastPane->mpUserData = dataPtr;
            break;
        case 'pas1':
            pParentPane = pLastPane;
            break;
        case 'pae1':
            pLastPane = pParentPane;
            pParentPane = pLastPane->GetParent();
            break;
        case 'grp1':
            if (!bReadRootGroup) {
                bReadRootGroup = true;
                mpGroupContainer = NewObj<GroupContainer>();
            } else if (mpGroupContainer && groupNestLevel == 1) {
                if (Group* pGroup = NewObj<Group>(static_cast<const res::Group*>(dataPtr), mpRootPane)) {
                    mpGroupContainer->AppendGroup(pGroup);
                }
            }
            break;
        case 'grs1':
            groupNestLevel++;
            break;
        case 'gre1':
            groupNestLevel--;
            break;
        }

        dataPtr = static_cast<const u8*>(dataPtr) + pDataBlockHead->size;
    }

    return true;
}

/* 0x8052BA60 (0x80): Creates an animation transform and keeps it in the layout's list. */
AnimTransform* Layout::CreateAnimTransform() {
    AnimTransformBasic* pAnimTrans = NewObj<AnimTransformBasic>();
    if (pAnimTrans) {
        mAnimTransList.PushBack(pAnimTrans);
    }
    return pAnimTrans;
}

/* 0x8052BAE0 (0x58): Creates an animation transform for an `RLAN` resource image. */
/* untyped: byte range - the RLAN resource image */
AnimTransform* Layout::CreateAnimTransform(const void* animResBuf, ResourceAccessor* pResAcsr) {
    AnimResource animRes;
    animRes.Set(animResBuf);
    return CreateAnimTransform(animRes, pResAcsr);
}

/* 0x8052BB38 (0x84): Creates an animation transform for an animation resource, or returns NULL when it has no
 * animation block. */
AnimTransform* Layout::CreateAnimTransform(const AnimResource& animRes, ResourceAccessor* pResAcsr) {
    const res::AnimationBlock* const pAnimBlock = animRes.GetResourceBlock();
    if (!pAnimBlock) {
        return NULL;
    }

    AnimTransform* const pAnimTrans = CreateAnimTransform();
    if (pAnimTrans) {
        pAnimTrans->SetResource(pAnimBlock, pResAcsr);
    }
    return pAnimTrans;
}

/* 0x8052BBBC (0x28): Binds an animation to the whole pane tree. */
void Layout::BindAnimation(AnimTransform* pAnimTrans) {
    if (mpRootPane) {
        mpRootPane->BindAnimation(pAnimTrans, true, false);
    }
}

/* 0x8052BBE4 (0x24): Unbinds an animation from the whole pane tree. */
void Layout::UnbindAnimation(AnimTransform* pAnimTrans) {
    if (mpRootPane) {
        mpRootPane->UnbindAnimation(pAnimTrans, true);
    }
}

/* 0x8052BC08 (0x14): Unbinds every animation from the pane tree. */
void Layout::UnbindAllAnimation() {
    UnbindAnimation(NULL);
}

/* 0x8052BC1C (0x29C): Creates an animation transform for a resource and binds it to the panes it names: the whole
 * tree, or the resource's groups, then the panes the resource's share records extend it to. */
bool Layout::BindAnimationAuto(const AnimResource& animRes, ResourceAccessor* pResAcsr) {
    if (!mpRootPane) {
        return false;
    }
    if (!animRes.GetResourceBlock()) {
        return false;
    }

    AnimTransform* const pAnimTrans = CreateAnimTransform();
    const u16 bindGroupNum = animRes.GetGroupNum();
    u16 animNum = 0;

    if (bindGroupNum == 0) {
        animNum = animRes.GetResourceBlock()->animContNum;
        pAnimTrans->SetResource(animRes.GetResourceBlock(), pResAcsr, animNum);
        mpRootPane->BindAnimation(pAnimTrans, true, true);
    } else {
        const res::AnimationGroupRef* const groupRefs = animRes.GetGroupArray();

        for (int grpIdx = 0; grpIdx < bindGroupNum; ++grpIdx) {
            Group* const pGroup = mpGroupContainer->FindGroupByName(groupRefs[grpIdx].name);
            animNum += animRes.CalcAnimationNum(pGroup, animRes.IsDescendingBind());
        }

        pAnimTrans->SetResource(animRes.GetResourceBlock(), pResAcsr, animNum);

        for (int grpIdx = 0; grpIdx < bindGroupNum; ++grpIdx) {
            Group* const pGroup = mpGroupContainer->FindGroupByName(groupRefs[grpIdx].name);
            detail::BindAnimation(pGroup, pAnimTrans, animRes.IsDescendingBind(), true);
        }
    }

    const u16 animShareInfoNum = animRes.GetAnimationShareInfoNum();
    if (animShareInfoNum > 0) {
        const res::AnimationShareInfo* const animShareInfoAry = animRes.GetAnimationShareInfoArray();

        for (int i = 0; i < animShareInfoNum; ++i) {
            Pane* const pSrcPane = mpRootPane->FindPaneByName(animShareInfoAry[i].srcPaneName, true);
            AnimPaneTree animPaneTree;
            animPaneTree.mAnimRes.Init();
            animPaneTree.Init();
            animPaneTree.Set(pSrcPane, animRes);
            if (animPaneTree.mLinkNum == 0) {
                continue;
            }

            Group* const pGroup = mpGroupContainer->FindGroupByName(animShareInfoAry[i].targetGroupName);
            PaneLinkList& paneList = pGroup->mPaneLinkList;

            for (PaneLinkList::Iterator it = paneList.GetBeginIter(); it != paneList.GetEndIter(); ++it) {
                if (it->mTarget != pSrcPane) {
                    if (bindGroupNum > 0) {
                        bool bInclude = IsIncludeAnimationGroupRef(mpGroupContainer, animRes.GetGroupArray(),
                                                                   bindGroupNum, animRes.IsDescendingBind(),
                                                                   it->mTarget);
                        if (!bInclude) {
                            continue;
                        }
                    }

                    animPaneTree.Bind(this, it->mTarget, pResAcsr);
                }
            }
        }
    }

    return true;
}

/* 0x8052BEB8 (0x24): Enables or disables an animation on the whole pane tree. */
void Layout::SetAnimationEnable(AnimTransform* pAnimTrans, bool bEnable) {
    if (mpRootPane) {
        mpRootPane->SetAnimationEnable(pAnimTrans, bEnable, true);
    }
}

/* 0x8052BEDC (0x20): Computes the pane tree's matrices. */
void Layout::CalculateMtx(const DrawInfo& drawInfo) {
    if (mpRootPane) {
        mpRootPane->CalculateMtx(drawInfo);
    }
}

/* 0x8052BEFC (0x20): Draws the pane tree. */
void Layout::Draw(const DrawInfo& drawInfo) {
    if (mpRootPane) {
        mpRootPane->Draw(drawInfo);
    }
}

/* 0x8052BF1C (0x20): Advances the pane tree's animations. */
void Layout::Animate(u32 option) {
    if (mpRootPane) {
        mpRootPane->Animate(option);
    }
}

/* 0x8052BF3C (0x38): Returns the layout's rectangle, centred on the origin. */
const ut::Rect Layout::GetLayoutRect() const {
    return ut::Rect(-mLayoutSize.width / 2, mLayoutSize.height / 2, mLayoutSize.width / 2, -mLayoutSize.height / 2);
}

/* 0x8052BF74 (0x1B0): Gives every text box in the pane tree the tag processor. */
void Layout::SetTagProcessor(ut::TagProcessorBase<wchar_t>* pTagProcessor) {
    SetTagProcessorImpl(mpRootPane, pTagProcessor);
}

/* 0x8052C124 (0x20C): Builds the pane a pane block describes, or returns NULL for another block. */
/* untyped: byte range - the pane block the builder reads */
Pane* Layout::BuildPaneObj(s32 kind, const void* dataPtr, const ResBlockSet& resBlockSet) {
    switch (kind) {
    case 'pan1': {
        const res::Pane* pResPane = static_cast<const res::Pane*>(dataPtr);
        return NewObj<Pane>(pResPane);
    }
    case 'pic1': {
        const res::Picture* pResPic = static_cast<const res::Picture*>(dataPtr);
        return NewObj<Picture>(pResPic, resBlockSet);
    }
    case 'txt1': {
        const res::TextBox* pResTextBox = static_cast<const res::TextBox*>(dataPtr);
        return NewObj<TextBox>(pResTextBox, resBlockSet);
    }
    case 'wnd1': {
        const res::Window* pResWindow = static_cast<const res::Window*>(dataPtr);
        return NewObj<Window>(pResWindow, resBlockSet);
    }
    case 'bnd1': {
        const res::Bounding* pResBounding = static_cast<const res::Bounding*>(dataPtr);
        return NewObj<Bounding>(pResBounding, resBlockSet);
    }
    default:
        return NULL;
    }
}

const ut::detail::RuntimeTypeInfo Picture::typeInfo(&Pane::typeInfo);

/* 0x8052C330 (0x14C): Builds the picture from its `pic1` block: the corner colours, the texture coordinates and
 * the material. */
Picture::Picture(const res::Picture* pResPic, const ResBlockSet& resBlockSet) : Pane(pResPic) {
    u8 texCoordNum = ut::Min<u8>(pResPic->texCoordNum, 8);

    if (texCoordNum > 0) {
        mTexCoordAry.Reserve(texCoordNum);
    }

    for (int i = 0; i < 4; ++i) {
        *reinterpret_cast<u32*>(&mVtxColors[i]) = pResPic->vtxCols[i];
    }

    if (texCoordNum > 0 && !mTexCoordAry.IsEmpty()) {
        mTexCoordAry.Copy(&pResPic[1], texCoordNum);
    }

    const u32* matOffsTbl = detail::ConvertOffsToPtr<u32>(resBlockSet.pMaterialList, 0xC);
    const res::Material* pResMaterial =
        detail::ConvertOffsToPtr<res::Material>(resBlockSet.pMaterialList, matOffsTbl[pResPic->materialIdx]);
    mpMaterial = Layout::NewObj<Material>(pResMaterial, resBlockSet);
}

/* 0x8052C47C (0xD8): Destroys the material and the texture coordinates. */
Picture::~Picture() {
    if (mpMaterial && !mpMaterial->mbUserAllocated) {
        Layout::DeleteObj(mpMaterial);
        mpMaterial = NULL;
    }

    mTexCoordAry.Free();
}

/* 0x8052C554 (0x1B4): Adds a texture to the material with its own coordinate generator; the first texture also
 * sizes a picture that has no size. */
void Picture::Append(const TexMap& texMap) {
    if (mpMaterial->GetTextureNum() >= mpMaterial->mGXMemCap.texMap ||
        mpMaterial->GetTextureNum() >= mpMaterial->mGXMemCap.texCoordGen) {
        return;
    }

    const u8 texMapIdx = mpMaterial->GetTextureNum();
    mpMaterial->SetTextureNum((u8)(texMapIdx + 1));
    mpMaterial->GetTexMapAry()[texMapIdx] = texMap;
    mpMaterial->SetTexCoordGenNum(mpMaterial->GetTextureNum());
    SetMaterialTexCoordGen(mpMaterial, texMapIdx, TexCoordGen());
    mTexCoordAry.SetSize(mpMaterial->GetTextureNum());

    const Size zeroSize(0.0f, 0.0f);
    bool bZeroSize = false;
    if (mSize.width == zeroSize.width && mSize.height == zeroSize.height) {
        bZeroSize = true;
    }

    if (bZeroSize && mpMaterial->GetTextureNum() == 1) {
        mSize = detail::GetTextureSize(mpMaterial, 0);
    }
}

/* 0x8052C708 (0x2C): Returns the colour of corner `idx`. */
ut::Color Picture::GetVtxColor(u32 idx) const {
    return mVtxColors[idx];
}

/* 0x8052C734 (0x2C): Sets the colour of corner `idx`. */
void Picture::SetVtxColor(u32 idx, ut::Color value) {
    mVtxColors[idx] = value;
}

/* 0x8052C760 (0x18): Returns one channel of a corner colour. */
u8 Picture::GetVtxColorElement(u32 idx) const {
    return reinterpret_cast<const u8*>(&mVtxColors[idx / 4])[idx % 4];
}

/* 0x8052C778 (0x18): Sets one channel of a corner colour. */
void Picture::SetVtxColorElement(u32 idx, u8 value) {
    reinterpret_cast<u8*>(&mVtxColors[idx / 4])[idx % 4] = value;
}

/* 0x8052C790 (0xDC): Draws the picture's quad with its material and corner colours. */
void Picture::DrawSelf(const DrawInfo& drawInfo) {
    if (!mpMaterial) {
        return;
    }

    LoadMtx(drawInfo);

    bool bUseVtxCol = mpMaterial->SetupGX(detail::IsModulateVertexColor(mVtxColors, mGlbAlpha), mGlbAlpha);
    detail::SetVertexFormat(bUseVtxCol, mTexCoordAry.GetSize());
    detail::DrawQuad(GetVtxPos(), mSize, mTexCoordAry.GetSize(), mTexCoordAry.GetArray(),
                     bUseVtxCol ? mVtxColors : NULL, mGlbAlpha);
}

/* 0x8052C86C (0x8): Returns the class's run-time type record. */
const ut::detail::RuntimeTypeInfo* Picture::GetRuntimeTypeInfo() const {
    return &typeInfo;
}

}  // namespace lyt
}  // namespace nw4r
