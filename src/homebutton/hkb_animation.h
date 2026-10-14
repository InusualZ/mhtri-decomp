/*
 * homebutton/hkb_animation.h - the keyboard layout engine's animation (nw4r::lyt): the RLAN animation resource
 *   records, the animation transform a pane or material is animated through, the resource reader, the per-pane
 *   animation tree, and the animation-link list helpers homebutton/hkb_material.cpp calls.
 *   The layouts are read from homebutton/hkb_animation.cpp (the constructor stores, Set, Bind and Animate).
 */
#ifndef HOMEBUTTON_HKB_ANIMATION_H
#define HOMEBUTTON_HKB_ANIMATION_H

#include "types.h"
#include "nw4r/fn_805012C4.h"

namespace nw4r {
namespace ut {
template <typename T, int Ofs>
class LinkList;
}  // namespace ut

namespace lyt {

class Pane;
class Group;
class Layout;
class Material;
class AnimationLink;
class ResourceAccessor;

typedef ut::LinkList<AnimationLink, 0> AnimationLinkList;

namespace res {

/* One key of a step curve: the frame and the value held from it on. size: 0x8 */
struct StepKey {
    /* +0x0 */ f32 frame;
    /* +0x4 */ u16 value;
    /* +0x6 */ u16 padding;
};

/* One key of a Hermite curve: the frame, the value and the slope there. size: 0xC */
struct HermiteKey {
    /* +0x0 */ f32 frame;
    /* +0x4 */ f32 value;
    /* +0x8 */ f32 slope;
};

/* One animated element: which record (id), which component (target) and its key array. size: 0xC */
struct AnimationTarget {
    /* +0x0 */ u8 id;
    /* +0x1 */ u8 target;
    /* +0x2 */ u8 curveType;
    /* +0x3 */ u8 padding1;
    /* +0x4 */ u16 keyNum;
    /* +0x6 */ u8 padding2[2];
    /* +0x8 */ u32 keysOffset;
};

/* One animation kind ('RLPA', 'RLVC', 'RLMC', ...) of a content; the target offsets follow it. size: 0x8 */
struct AnimationInfo {
    /* +0x0 */ u32 kind;
    /* +0x4 */ u8 num;
    /* +0x5 */ u8 padding[3];
};

/* The animations of one pane (type 0) or material (type 1); the info offsets follow it. size: 0x18 */
struct AnimationContent {
    /* +0x00 */ char name[20];
    /* +0x14 */ u8 num;
    /* +0x15 */ u8 type;
    /* +0x16 */ u8 padding[2];
};

/* The `pai1` block: frame count, the referenced texture files and the content offsets. size: 0x14 */
struct AnimationBlock {
    /* +0x00 */ ut::BinaryBlockHeader blockHeader;
    /* +0x08 */ u16 frameSize;
    /* +0x0A */ u8 loop;
    /* +0x0B */ u8 padding1;
    /* +0x0C */ u16 fileNum;
    /* +0x0E */ u16 animContNum;
    /* +0x10 */ u32 animContOffsetsOffset;
};

/* The `pat1` block: the tag's group references and its bind flag. size: 0x1C */
struct AnimationTagBlock {
    /* +0x00 */ ut::BinaryBlockHeader blockHeader;
    /* +0x08 */ u16 tagOrder;
    /* +0x0A */ u16 groupNum;
    /* +0x0C */ u32 nameOffset;
    /* +0x10 */ u32 groupsOffset;
    /* +0x14 */ s16 startFrame;
    /* +0x16 */ s16 endFrame;
    /* +0x18 */ u8 flag;
    /* +0x19 */ u8 padding[3];
};

/* The `pah1` block: the animation share records. size: 0x10 */
struct AnimationShareBlock {
    /* +0x00 */ ut::BinaryBlockHeader blockHeader;
    /* +0x08 */ u32 animShareInfoOffset;
    /* +0x0C */ u16 shareNum;
    /* +0x0E */ u8 padding[2];
};

struct AnimationGroupRef;
struct AnimationShareInfo;

}  // namespace res

/* An animation that can be bound to panes and materials and applied per frame. size: 0x14 */
class AnimTransform {
public:
    AnimTransform() : mpRes(NULL), mFrame(0.0f) {}
    virtual ~AnimTransform() {}
    virtual void SetResource(const res::AnimationBlock* pRes, ResourceAccessor* pResAccessor) = 0;
    virtual void SetResource(const res::AnimationBlock* pRes, ResourceAccessor* pResAccessor, u16 animNum) = 0;
    virtual void Bind(Pane* pPane, bool bRecursive, bool bDisable) = 0;
    virtual void Bind(Material* pMaterial, bool bDisable) = 0;
    virtual void Animate(u32 idx, Pane* pPane) = 0;
    virtual void Animate(u32 idx, Material* pMaterial) = 0;

    u16 GetFrameSize() const;
    const res::AnimationBlock* GetAnimResource() const { return mpRes; }

    /* +0x04 */ ut::detail::LinkListNode mLink;
    /* +0x0C */ const res::AnimationBlock* mpRes;
    /* +0x10 */ f32 mFrame;
};

/* The animation transform of one `pai1` block: the texture files it loads and its animation links. size: 0x20 */
class AnimTransformBasic : public AnimTransform {
public:
    AnimTransformBasic();
    virtual ~AnimTransformBasic();
    virtual void SetResource(const res::AnimationBlock* pRes, ResourceAccessor* pResAccessor);
    virtual void SetResource(const res::AnimationBlock* pRes, ResourceAccessor* pResAccessor, u16 animNum);
    virtual void Bind(Pane* pPane, bool bRecursive, bool bDisable);
    virtual void Bind(Material* pMaterial, bool bDisable);
    virtual void Animate(u32 idx, Pane* pPane);
    virtual void Animate(u32 idx, Material* pMaterial);

    AnimationLink* FindUnbindLink(AnimationLink* pLink) const;
    template <typename T>
    AnimationLink* Bind(T* pTarget, AnimationLink* pAnimLink, u16 idx, bool bDisable);

    /* +0x14 */ void** mpFileResAry; /* untyped: the 'timg' resources, one TPL file each */
    /* +0x18 */ AnimationLink* mAnimLinkAry;
    /* +0x1C */ u16 mAnimLinkNum;
};

/* An RLAN animation file and its `pai1`, `pat1` and `pah1` blocks. size: 0x10 */
class AnimResource {
public:
    /* untyped: byte range - the RLAN file image */
    void Set(const void* anmResBuf);
    void Init();
    u16 GetGroupNum() const;
    const res::AnimationGroupRef* GetGroupArray() const;
    bool IsDescendingBind() const;
    u16 GetAnimationShareInfoNum() const;
    const res::AnimationShareInfo* GetAnimationShareInfoArray() const;
    u16 CalcAnimationNum(Pane* pPane, bool bRecursive) const;
    u16 CalcAnimationNum(Group* pGroup, bool bRecursive) const;

    const res::AnimationBlock* GetResourceBlock() const { return mpResBlock; }

    /* +0x00 */ const ut::BinaryFileHeader* mpFileHeader;
    /* +0x04 */ const res::AnimationBlock* mpResBlock;
    /* +0x08 */ const res::AnimationTagBlock* mpTagBlock;
    /* +0x0C */ const res::AnimationShareBlock* mpShareBlock;
};

/* The contents of one animation resource that target a pane and its materials. size: 0x28 */
class AnimPaneTree {
public:
    enum {
        MATERIAL_NUM_MAX = 9
    };

    void Init();
    void Set(Pane* pTargetPane, const AnimResource& animRes);
    AnimTransform* Bind(Layout* pLayout, Pane* pTargetPane, ResourceAccessor* pResAccessor) const;

    /* +0x00 */ AnimResource mAnimRes;
    /* +0x10 */ u16 mAnimPaneIdx;
    /* +0x12 */ u16 mLinkNum;
    /* +0x14 */ u16 mAnimMatIdxs[MATERIAL_NUM_MAX];
    /* +0x26 */ u8 mAnimMatCnt;
};

namespace detail {

AnimationLink* FindAnimationLink(AnimationLinkList* pAnimList, AnimTransform* pAnimTrans);
AnimationLink* FindAnimationLink(AnimationLinkList* pAnimList, const AnimResource& animRes);
void UnbindAnimationLink(AnimationLinkList* pAnimList, AnimTransform* pAnimTrans);

}  // namespace detail

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_ANIMATION_H */
