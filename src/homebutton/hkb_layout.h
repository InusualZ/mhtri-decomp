/*
 * homebutton/hkb_layout.h - the keyboard layout engine's layout (nw4r::lyt::Layout), its groups (Group,
 *   GroupContainer) and its picture pane (Picture), which homebutton/hkb_layout.cpp defines; the run-time type check
 *   and the binary resource records the builders read; and the pane base class (nw4r::lyt::Pane, whose members
 *   homebutton/hkb_pane.cpp defines) the pane units derive from.
 */
#ifndef HOMEBUTTON_HKB_LAYOUT_H
#define HOMEBUTTON_HKB_LAYOUT_H

#include "types.h"
#include "homebutton/hkb_animation.h"
#include "homebutton/hkb_drawinfo.h"
#include "homebutton/hkb_keyboard.h"
#include "homebutton/hkb_material.h"
#include "homebutton/hkb_resource_accessor.h"
#include "MEM/MEMAllocFromAllocator.h"

namespace nw4r {
namespace ut {
namespace detail {

/* A class's run-time type record: the parent class's record, or NULL for a root class. size: 0x4 */
struct RuntimeTypeInfo {
    RuntimeTypeInfo(const RuntimeTypeInfo* pParent) : mParentTypeInfo(pParent) {}

    /* Returns whether this class is `pType` or derives from it. */
    bool IsDerivedFrom(const RuntimeTypeInfo* pType) const {
        for (const RuntimeTypeInfo* pInfo = this; pInfo != NULL; pInfo = pInfo->mParentTypeInfo) {
            if (pInfo == pType) {
                return true;
            }
        }
        return false;
    }

    /* +0x0 */ const RuntimeTypeInfo* mParentTypeInfo;
};

}  // namespace detail

/* Returns `pObj` as a T when its run-time class is T or derives from it, else NULL. */
template <typename T, typename U>
inline T* DynamicCast(U* pObj) {
    const detail::RuntimeTypeInfo* pTypeInfo = &T::typeInfo;

    if (pObj != NULL && pObj->GetRuntimeTypeInfo()->IsDerivedFrom(pTypeInfo)) {
        return static_cast<T*>(pObj);
    }
    return NULL;
}

}  // namespace ut

namespace lyt {

class Pane;
class Group;
class DrawInfo;
class Layout;

namespace res {

/* The `pan1` block: the common pane record every pane block starts with. size: 0x4C */
struct Pane {
    /* +0x00 */ ut::BinaryBlockHeader blockHeader;
    /* +0x08 */ u8 flag;
    /* +0x09 */ u8 basePosition;
    /* +0x0A */ u8 alpha;
    /* +0x0B */ u8 pad_0xB;
    /* +0x0C */ char name[16];
    /* +0x1C */ char userData[8];
    /* +0x24 */ math::VEC3 translate;
    /* +0x30 */ math::VEC3 rotate;
    /* +0x3C */ math::VEC2 scale;
    /* +0x44 */ Size size;
};

/* The `pic1` block: a pane, its corner colours, its material and the texture coordinates that follow. size: 0x60 */
struct Picture : public Pane {
    /* +0x4C */ u32 vtxCols[4];
    /* +0x5C */ u16 materialIdx;
    /* +0x5E */ u8 texCoordNum;
    /* +0x5F */ u8 pad_0x5F;
};

/* The `bnd1` block: a pane with no record of its own. size: 0x4C */
struct Bounding : public Pane {};

/* The `grp1` block: the group name and the count of the pane names that follow it. size: 0x1C */
struct Group {
    /* +0x00 */ ut::BinaryBlockHeader blockHeader;
    /* +0x08 */ char name[16];
    /* +0x18 */ u16 paneNum;
    /* +0x1A */ u8 pad_0x1A[2];
};

/* The `lyt1` block: the layout's drawing origin flag and size. size: 0x14 */
struct Layout {
    /* +0x00 */ ut::BinaryBlockHeader blockHeader;
    /* +0x08 */ u8 originType;
    /* +0x09 */ u8 pad_0x9[3];
    /* +0x0C */ Size layoutSize;
};

}  // namespace res

/* A node of the layout's pane tree: its transform, size, alpha, name, material and child panes. size: 0xD8 */
class Pane {
public:
    Pane(const res::Pane* pBlock);
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
    virtual void BindAnimation(AnimTransform* pAnimTrans, bool bRecursive, bool bDisable);
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

    void AppendChild(Pane* pChild);
    void AddAnimationLink(AnimationLink* pAnimationLink);
    const math::VEC2 GetVtxPos() const;
    const ut::Rect GetPaneRect(const DrawInfo& drawInfo) const;

    const char* GetName() const { return mName; }
    void SetSRTElement(u32 idx, f32 value) { reinterpret_cast<f32*>(&mTranslate)[idx] = value; }
    void SetVisible(bool bVisible) { mFlag = (u8)((mFlag & (u8)~1) | (bVisible ? 1 : 0)); }

    Pane* GetParent() const { return mpParent; }
    const Size& GetSize() const { return mSize; }
    u8 GetAlpha() const { return mAlpha; }
    u8 GetGlobalAlpha() const { return mGlbAlpha; }
    bool IsVisible() const { return (mFlag & 1) != 0; }
    bool IsUserAllocated() const { return mbUserAllocated; }

    /* The class's run-time type record (0x80795A18); its parent is NULL. */
    static const ut::detail::RuntimeTypeInfo typeInfo;

    /* +0x04 */ ut::detail::LinkListNode mLink;
    /* +0x0C */ Pane* mpParent;
    /* +0x10 */ ut::LinkList<Pane, 4> mChildList;
    /* +0x1C */ AnimationLinkList mAnimList;
    /* +0x28 */ Material* mpMaterial;
    /* +0x2C */ math::VEC3 mTranslate;
    /* +0x38 */ math::VEC3 mRotate;
    /* +0x44 */ math::VEC2 mScale;
    /* +0x4C */ Size mSize;
    /* +0x54 */ math::MTX34 mMtx;
    /* +0x84 */ math::MTX34 mGlbMtx;
    /* +0xB4 */ const void* mpUserData; /* untyped: the `usd1` block, read by the game */
    /* +0xB8 */ u8 mAlpha;
    /* +0xB9 */ u8 mGlbAlpha;
    /* +0xBA */ u8 mBasePosition;
    /* +0xBB */ u8 mFlag;
    /* +0xBC */ char mName[17];
    /* +0xCD */ char mUserData[9];
    /* +0xD6 */ bool mbUserAllocated;
    /* +0xD7 */ u8 pad_0xD7;
};

namespace detail {

/* Returns the pane's rectangle from its base point and size (homebutton/hkb_pane.cpp). */
const ut::Rect CalcPaneRect(const Pane* pPane);

}  // namespace detail

namespace res {

/* One group an animation binds to, by name. size: 0x14 */
struct AnimationGroupRef {
    /* +0x00 */ char name[17];
    /* +0x11 */ u8 flag;
    /* +0x12 */ u8 pad_0x12[2];
};

/* One pane whose animation is shared with the panes of a group. size: 0x24 */
struct AnimationShareInfo {
    /* +0x00 */ char srcPaneName[17];
    /* +0x11 */ char targetGroupName[17];
    /* +0x22 */ u8 pad_0x22[2];
};

}  // namespace res

/* One member of a group: the pane it names. size: 0xC */
struct PaneLink {
    /* +0x0 */ ut::detail::LinkListNode mLink;
    /* +0x8 */ Pane* mTarget;
};

/* A named set of panes from a `grp1` block. size: 0x2C */
class Group {
public:
    Group(const res::Group* pResGroup, Pane* pRootPane);
    virtual ~Group();

    void AppendPane(Pane* pPane);
    const char* GetName() const { return mName; }
    ut::LinkList<PaneLink, 0>* GetPaneList() { return &mPaneLinkList; }
    bool IsUserAllocated() const { return mbUserAllocated; }

    /* +0x04 */ ut::detail::LinkListNode mLink;
    /* +0x0C */ ut::LinkList<PaneLink, 0> mPaneLinkList;
    /* +0x18 */ char mName[17];
    /* +0x29 */ bool mbUserAllocated;
    /* +0x2A */ u8 pad_0x2A[2];
};

/* The layout's groups. size: 0xC */
class GroupContainer {
public:
    ~GroupContainer();

    void AppendGroup(Group* pGroup);
    Group* FindGroupByName(const char* findName);

    /* +0x0 */ ut::LinkList<Group, 4> mGroupList;
};

/* A layout: the pane tree built from a `RLYT` resource, its groups and its animations. size: 0x20 */
class Layout {
public:
    Layout();
    virtual ~Layout();
    /* untyped: byte range - the RLYT resource image */
    virtual bool Build(const void* lytResBuf, ResourceAccessor* pResAcsr);
    virtual AnimTransform* CreateAnimTransform();
    /* untyped: byte range - the RLAN resource image */
    virtual AnimTransform* CreateAnimTransform(const void* animResBuf, ResourceAccessor* pResAcsr);
    virtual AnimTransform* CreateAnimTransform(const AnimResource& animRes, ResourceAccessor* pResAcsr);
    virtual void BindAnimation(AnimTransform* pAnimTrans);
    virtual void UnbindAnimation(AnimTransform* pAnimTrans);
    virtual void UnbindAllAnimation();
    virtual bool BindAnimationAuto(const AnimResource& animRes, ResourceAccessor* pResAcsr);
    virtual void SetAnimationEnable(AnimTransform* pAnimTrans, bool bEnable);
    virtual void CalculateMtx(const DrawInfo& drawInfo);
    virtual void Draw(const DrawInfo& drawInfo);
    virtual void Animate(u32 option);
    virtual void SetTagProcessor(ut::TagProcessorBase<wchar_t>* pTagProcessor);

    const ut::Rect GetLayoutRect() const;
    /* untyped: byte range - the pane block the builder reads */
    static Pane* BuildPaneObj(s32 kind, const void* dataPtr, const ResBlockSet& resBlockSet);

    /* Allocates `size` bytes from the layout allocator. */
    /* untyped: byte range - the raw block */
    static void* AllocMemory(u32 size) { return MEMAllocFromAllocator(mspAllocator, size); }
    /* Returns a block to the layout allocator. */
    /* untyped: byte range - the raw block */
    static void FreeMemory(void* ptr) { MEMFreeToAllocator(mspAllocator, ptr); }

    /* Constructs a T in a block from the layout allocator, or returns NULL. */
    template <typename T>
    static T* NewObj() {
        void* pMem = AllocMemory(sizeof(T)); /* untyped: the raw block */
        if (pMem) {
            return new (pMem) T();
        }
        return NULL;
    }

    /* Constructs a T(param1) in a block from the layout allocator, or returns NULL. */
    template <typename T, typename Param1>
    static T* NewObj(Param1 param1) {
        void* pMem = AllocMemory(sizeof(T)); /* untyped: the raw block */
        if (pMem) {
            return new (pMem) T(param1);
        }
        return NULL;
    }

    /* Constructs a T(param1, param2) in a block from the layout allocator, or returns NULL. */
    template <typename T, typename Param1, typename Param2>
    static T* NewObj(Param1 param1, Param2 param2) {
        void* pMem = AllocMemory(sizeof(T)); /* untyped: the raw block */
        if (pMem) {
            return new (pMem) T(param1, param2);
        }
        return NULL;
    }

    /* Destroys a T made by NewObj and returns its block. */
    template <typename T>
    static void DeleteObj(T* pObj) {
        if (pObj) {
            pObj->~T();
            FreeMemory(pObj);
        }
    }

    /* Constructs `num` default T in one block from the layout allocator, or returns NULL. */
    template <typename T>
    static T* NewArray(u32 num) {
        T* objAry = static_cast<T*>(AllocMemory(sizeof(T) * num));
        if (!objAry) {
            return NULL;
        }
        for (u32 i = 0; i < num; ++i) {
            new (&objAry[i]) T();
        }
        return objAry;
    }

    /* Returns a block NewArray allocated, without destroying its elements. */
    template <typename T>
    static void DeleteArray(T* objAry) {
        if (objAry) {
            FreeMemory(objAry);
        }
    }

    /* Destroys `num` T made by NewArray and returns their block. */
    template <typename T>
    static void DeleteArray(T* objAry, u32 num) {
        if (objAry) {
            for (u32 i = 0; i < num; ++i) {
                objAry[i].~T();
            }
            FreeMemory(objAry);
        }
    }

    /* The allocator every layout object comes from (0x80795A20). */
    static MEMAllocator* mspAllocator;

    /* +0x04 */ ut::LinkList<AnimTransform, 4> mAnimTransList;
    /* +0x10 */ Pane* mpRootPane;
    /* +0x14 */ GroupContainer* mpGroupContainer;
    /* +0x18 */ Size mLayoutSize;
};

/* A pane drawn as one textured, vertex-coloured quad. size: 0xF0 */
class Picture : public Pane {
public:
    Picture(const res::Picture* pResPic, const ResBlockSet& resBlockSet);
    virtual ~Picture();
    virtual const ut::detail::RuntimeTypeInfo* GetRuntimeTypeInfo() const;
    virtual void DrawSelf(const DrawInfo& drawInfo);
    virtual ut::Color GetVtxColor(u32 idx) const;
    virtual void SetVtxColor(u32 idx, ut::Color value);
    virtual u8 GetVtxColorElement(u32 idx) const;
    virtual void SetVtxColorElement(u32 idx, u8 value);
    virtual void Append(const TexMap& texMap);

    /* The class's run-time type record (0x80795A28); its parent is Pane's. */
    static const ut::detail::RuntimeTypeInfo typeInfo;

    /* +0xD8 */ ut::Color mVtxColors[4];
    /* +0xE8 */ detail::TexCoordAry mTexCoordAry;
};

}  // namespace lyt
}  // namespace nw4r

#endif
