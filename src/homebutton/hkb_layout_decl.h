/*
 * homebutton/hkb_layout_decl.h - the keyboard layout engine's layout, pane and group classes (nw4r::lyt::Layout,
 *   Pane, Group) as their callers see them: the shared layout allocator, the animation factory, the pane virtual
 *   interface and the fields the animation code writes.  The definitions are homebutton/fn_8052A040.cpp's
 *   (Layout::mspAllocator .sbss 0x80795A20, Pane::AddAnimationLink 0x8052AC34).
 */
#ifndef HOMEBUTTON_HKB_LAYOUT_DECL_H
#define HOMEBUTTON_HKB_LAYOUT_DECL_H

#include "types.h"
#include "homebutton/hkb_animation.h"
#include "homebutton/hkb_drawinfo.h"
#include "homebutton/hkb_material.h"
#include "MEM/MEMAllocFromAllocator.h"

namespace nw4r {
namespace ut {
namespace detail {
struct RuntimeTypeInfo;
}  // namespace detail
}  // namespace ut

namespace lyt {

/* A node of the layout's pane tree. size: 0xD8 (approximation: only mAnimList, the SRT block, mFlag and mName
 * are read here) */
class Pane {
public:
    virtual ~Pane();
    virtual const ut::detail::RuntimeTypeInfo* GetRuntimeTypeInfo() const;
    virtual void CalculateMtx(const DrawInfo& drawInfo);
    virtual void Draw(const DrawInfo& drawInfo);
    virtual void DrawSelf(const DrawInfo& drawInfo);
    virtual void Animate(u32 option);
    virtual void AnimateSelf(u32 option);
    virtual ut::Color GetVtxColor(u32 idx) const;
    virtual void SetVtxColor(u32 idx, ut::Color value);
    virtual u8 GetColorElement(u32 idx) const;
    virtual void SetColorElement(u32 idx, u8 value);
    virtual u8 GetVtxColorElement(u32 idx) const;
    virtual void SetVtxColorElement(u32 idx, u8 value);
    virtual Pane* FindPaneByName(const char* findName, bool bRecursive);
    virtual Material* FindMaterialByName(const char* findName, bool bRecursive);
    virtual void BindAnimation(AnimTransform* pAnimTrans, bool bRecursive);
    virtual void UnbindAnimation(AnimTransform* pAnimTrans, bool bRecursive);
    virtual void UnbindAllAnimation(bool bRecursive);
    virtual void UnbindAnimationSelf(AnimTransform* pAnimTrans);
    virtual AnimationLink* FindAnimationLinkSelf(AnimTransform* pAnimTrans);
    virtual AnimationLink* FindAnimationLinkSelf(const AnimResource& animRes);
    virtual void SetAnimationEnable(AnimTransform* pAnimTrans, bool bEnable, bool bRecursive);
    virtual void SetAnimationEnable(const AnimResource& animRes, bool bEnable, bool bRecursive);
    virtual u8 GetMaterialNum() const;
    virtual Material* GetMaterial() const;
    virtual Material* GetMaterial(u32 idx) const;
    virtual void LoadMtx(const DrawInfo& drawInfo);

    void AddAnimationLink(AnimationLink* pAnimationLink);

    const char* GetName() const { return mName; }
    void SetSRTElement(u32 idx, f32 value) { reinterpret_cast<f32*>(&mTranslate)[idx] = value; }
    void SetVisible(bool bVisible) { mFlag = (u8)((mFlag & (u8)~1) | (bVisible ? 1 : 0)); }

    /* +0x04 */ ut::detail::LinkListNode mLink;
    /* +0x0C */ Pane* mpParent;
    /* +0x10 */ ut::LinkList<Pane, 4> mChildList;
    /* +0x1C */ AnimationLinkList mAnimList;
    /* +0x28 */ Material* mpMaterial;
    /* +0x2C */ math::VEC3 mTranslate;
    /* +0x38 */ math::VEC3 mRotate;
    /* +0x44 */ math::VEC2 mScale;
    /* +0x4C */ math::VEC2 mSize;
    /* +0x54 */ math::MTX34 mMtx;
    /* +0x84 */ math::MTX34 mGlbMtx;
    /* +0xB4 */ u8 pad_0xB4[4];
    /* +0xB8 */ u8 mAlpha;
    /* +0xB9 */ u8 mGlbAlpha;
    /* +0xBA */ u8 mBasePosition;
    /* +0xBB */ u8 mFlag;
    /* +0xBC */ char mName[17];
    /* +0xCD */ char mUserData[9];
    /* +0xD6 */ bool mbUserAllocated;
    /* +0xD7 */ u8 pad_0xD7;
};

/* One pane of a group. size: 0xC */
struct PaneLink {
    /* +0x0 */ ut::detail::LinkListNode mLink;
    /* +0x8 */ Pane* mTarget;
};

/* A named set of panes. size: 0x10 (approximation: only the pane list is read here) */
class Group {
public:
    virtual ~Group();

    ut::LinkList<PaneLink, 0>* GetPaneList() { return &mPaneLinkList; }

    /* +0x04 */ ut::detail::LinkListNode mLink;
    /* +0x0C */ ut::LinkList<PaneLink, 0> mPaneLinkList;
};

/* A layout: its pane tree and resources; the shared allocator and the animation factory are used here.
 * size: 0x4 (approximation: no instance field is read) */
class Layout {
public:
    virtual ~Layout();
    /* untyped: byte range - the RLYT file image */
    virtual bool Build(const void* lytResBuf, ResourceAccessor* pResAcsr);
    virtual AnimTransform* CreateAnimTransform();

    /* untyped: byte range - a block from the layout allocator */
    static void* AllocMemory(u32 size) { return MEMAllocFromAllocator(mspAllocator, size); }
    /* untyped: byte range - a block from the layout allocator */
    static void FreeMemory(void* ptr) { MEMFreeToAllocator(mspAllocator, ptr); }

    /* Allocates and default-constructs `num` objects, or returns NULL. */
    template <typename T>
    static T* NewArray(u32 num) {
        T* pArray = static_cast<T*>(AllocMemory(num * sizeof(T)));
        if (!pArray) {
            return NULL;
        }
        for (u32 i = 0; i < num; ++i) {
            new (&pArray[i]) T();
        }
        return pArray;
    }

    /* Frees an array NewArray allocated. */
    template <typename T>
    static void DeleteArray(T* pArray) {
        if (pArray) {
            FreeMemory(pArray);
        }
    }

    /* The allocator every layout object comes from. */
    static MEMAllocator* mspAllocator; /* 0x80795A20 */
};

}  // namespace lyt
}  // namespace nw4r

#endif /* HOMEBUTTON_HKB_LAYOUT_DECL_H */
