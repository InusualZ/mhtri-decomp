/* homebutton/hkb_layout.cpp - the software keyboard layout engine's layout (nw4r::lyt::Layout: it walks an `RLYT`
 *   resource's blocks and builds the pane tree, the groups and the animations).
 * RANGE. .text 0x8052B344-0x8052C330 (21 functions); .data 0x8064A5F8-0x8064A638 (Layout's class table);
 *   .sbss 0x80795A20-0x80795A28 (Layout::mspAllocator); .sdata2 0x8079D570-0x8079D578 (0.0 and a second constant).
 *   Edges: the left edge is homebutton/hkb_group.cpp's last function (0x8052B2C8..0x8052B344); the right edge is
 *   homebutton/hkb_picture.cpp's first function (the Picture constructor) at 0x8052C330. The seam evidence is in
 *   homebutton/hkb_group.cpp.
 * FLAGS. GC/3.0a5.2 + cflags_hkb, the band's flags (evidence: src/homebutton/hbm_kb_event.cpp FLAGS).
 * NAMES. GUESS: nw4r::lyt::Layout and its members, from the runtime dump (nw4hbm::lyt::Layout::UnbindAnimation,
 *   SetAnimationEnable, Animate, BuildPaneObj at their addresses). SetTagProcessorImpl and
 *   IsIncludeAnimationGroupRef are GUESS names for the file's static helpers.
 * SHAPES. the Layout constructor keeps LinkListNode's two zero stores before the list initialisation because
 *   LinkListNode derives from ut::NonCopyable (src/nw4r/ut_LinkList.h).
 * RESIDUALS. our object emits weak LinkList<...>, LinkListNode and ut::NonCopyable destructors the target lacks
 *   (unreferenced, dead-stripped by the link).
 * FLAGS. SetTagProcessor and SetTagProcessorImpl take a scoped `#pragma opt_loop_invariants off` (the second inlined level then keeps
 *   retail's `lwz` + `addi` child walk; same sizes, rows 100 %).
 */
#include "homebutton/hkb_layout.h"

#include "homebutton/hkb_bounding.h"
#include "homebutton/hkb_textbox.h"
#include "homebutton/hkb_window.h"

namespace nw4r {
namespace lyt {

typedef ut::LinkList<Pane, 4> PaneList;
typedef ut::LinkList<PaneLink, 0> PaneLinkList;
typedef ut::LinkList<AnimTransform, 4> AnimTransformList;

namespace {

#pragma push
#pragma opt_loop_invariants off
/* 0x8052B344 (0x1AC): Gives every text box in the pane's subtree the tag processor. */
inline void SetTagProcessorImpl(Pane* pPane, ut::TagProcessorBase<wchar_t>* pTagProcessor) {
    if (TextBox* pTextBox = ut::DynamicCast<TextBox>(pPane)) {
        pTextBox->mpTagProcessor = pTagProcessor;
    }

    for (PaneList::Iterator it = pPane->mChildList.GetBeginIter(); it != pPane->mChildList.GetEndIter(); ++it) {
        SetTagProcessorImpl(it.operator->(), pTagProcessor);
    }
}
#pragma pop

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

#pragma push
#pragma opt_loop_invariants off
/* 0x8052BF74 (0x1B0): Gives every text box in the pane tree the tag processor. */
void Layout::SetTagProcessor(ut::TagProcessorBase<wchar_t>* pTagProcessor) {
    SetTagProcessorImpl(mpRootPane, pTagProcessor);
}
#pragma pop

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

}  // namespace lyt
}  // namespace nw4r
