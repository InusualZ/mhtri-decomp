/* g3d/g3d_anmtexsrt.h - nw4r g3d's texture-SRT animation classes (AnmObjTexSrt and its node, override and resource
 *   forms), whose members `g3d/fn_800680CC.cpp` defines (the `g3d_anmtexsrt.cpp` part of its range).  Evidence: the
 *   four vtables at .data 0x8058D4D8..0x8058D5E0 and the `AnmObjTexSrt::Attach(...)` / `AnmObjTexSrtNode::G3dProc(...)`
 *   warnings. */
#ifndef MHTRI_G3D_G3D_ANMTEXSRT_H
#define MHTRI_G3D_G3D_ANMTEXSRT_H

#include "types.h"
#include "g3d/g3d_obj.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class AnmObjTexSrtRes;
struct TexSrtAnmResult;

/* The texture-SRT animation interface: one binding word per material (bit 15: no animation, bit 14: undefined).
 * size: 0x18 */
class AnmObjTexSrt : public AnmObj {
public:
    AnmObjTexSrt(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual ~AnmObjTexSrt();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame) = 0;
    virtual f32 GetFrame() const = 0;
    virtual void UpdateFrame() = 0;
    virtual void SetUpdateRate(f32 rate) = 0;
    virtual f32 GetUpdateRate() const = 0;
    virtual bool Bind(ResMdl mdl) = 0;
    virtual void Release();

    virtual const TexSrtAnmResult* GetResult(TexSrtAnmResult* pResult, u32 idx) = 0;
    virtual AnmObjTexSrtRes* Attach(int idx, AnmObjTexSrtRes* pRes);
    virtual AnmObjTexSrtRes* Detach(int idx);
    virtual void DetachAll();

    bool TestExistence(u32 idx) const;
    bool TestDefined(u32 idx) const;
    static const TypeObj GetTypeObjStatic();

    /* +0x10 */ int mNumBinding;
    /* +0x14 */ u16* mpBinding;
};

/* A texture-SRT animation made of child animations.  size: 0x20 */
class AnmObjTexSrtNode : public AnmObjTexSrt {
public:
    AnmObjTexSrtNode(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding, AnmObjTexSrtRes** ppChildrenBuf,
                     int numChildren);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual ~AnmObjTexSrtNode();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame);
    virtual f32 GetFrame() const;
    virtual void UpdateFrame();
    virtual void SetUpdateRate(f32 rate);
    virtual f32 GetUpdateRate() const;
    virtual bool Bind(ResMdl mdl);
    virtual void Release();

    virtual const TexSrtAnmResult* GetResult(TexSrtAnmResult* pResult, u32 idx) = 0;
    virtual AnmObjTexSrtRes* Attach(int idx, AnmObjTexSrtRes* pRes);
    virtual AnmObjTexSrtRes* Detach(int idx);
    virtual void DetachAll();

    static const TypeObj GetTypeObjStatic();

    /* +0x18 */ int mChildrenArraySize;
    /* +0x1C */ AnmObjTexSrtRes** mpChildrenArray;
};

/* A node whose later children override the earlier ones.  size: 0x20 */
class AnmObjTexSrtOverride : public AnmObjTexSrtNode {
public:
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual ~AnmObjTexSrtOverride();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual const TexSrtAnmResult* GetResult(TexSrtAnmResult* pResult, u32 idx);

    static const TypeObj GetTypeObjStatic();
};

/* A texture-SRT animation played from a resource.  Only the members its written functions reach are declared.
 * size: 0x34 (approximation: the resource form's tail is not reached yet) */
class AnmObjTexSrtRes : public AnmObjTexSrt {
public:
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual ~AnmObjTexSrtRes();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    static const TypeObj GetTypeObjStatic();

    /* +0x18 */ FrameCtrl mFrameCtrl;
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_ANMTEXSRT_H */
