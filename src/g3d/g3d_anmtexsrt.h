/* g3d/g3d_anmtexsrt.h - nw4r g3d's texture-SRT animation classes (AnmObjTexSrt and its node, override and resource
 *   forms), whose members `g3d/g3d_anmtexsrt.cpp` defines, and that unit's C-linkage helpers.  Evidence: the four
 *   vtables at .data 0x8058D4D8..0x8058D5E0 and the `AnmObjTexSrt::Attach(...)` / `AnmObjTexSrtNode::G3dProc(...)`
 *   warnings. */
#ifndef MHTRI_G3D_G3D_ANMTEXSRT_H
#define MHTRI_G3D_G3D_ANMTEXSRT_H

#include "types.h"
#include "g3d/g3d_obj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006CDBC - the checked `ResAnmTexSrt` resolver the `g3d/g3d_resfile.cpp` getters and the
 * `g3d/g3d_resanmtexsrt.cpp` field readers call. */
u32 fn_8006CDBC(void* p);
/* 0x8006D9FC - the indexed handle reader the `g3d_resanmtexsrt.cpp` field readers call. */
u32 fn_8006D9FC(void* self, u32 idx);
/* 0x8006D9A4 - material `idx`'s animation record, from the resource's material dictionary. */
u32 fn_8006D9A4(void* self, u32 idx);
/* 0x8006CE20 - the unchecked `ResAnmTexSrt` block pointer (`ptr()`). */
/* untyped: opaque handle - the `ResAnmTexSrt` handle, as the owner's definition spells it */
u32 res_anm_tex_srt_ptr(void* p);

#ifdef __cplusplus
}

struct ResHandle;
namespace nw4r { namespace g3d { struct TexSrtAnmResult; } }

/* 0x8006E338 (0x330): applies a texture-SRT result to a material's SRT block and indirect matrices. */
extern "C" void apply_tex_srt_anm_result(struct ResHandle texSrt, struct ResHandle indMtx,
                                         const nw4r::g3d::TexSrtAnmResult* pResult);
#endif

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class AnmObjTexSrtRes;
struct TexSrtAnmResult;

/* A texture-SRT animation resource block: the material dictionary, the frame and material counts and the play
 * policy.  size: 0x2C (approximation: only the read fields are named) */
struct ResAnmTexSrtData {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ s32 toTexSrtDataDic;
    /* +0x14 */ u8 pad_0x14[0xC];
    /* +0x20 */ u16 numFrame;
    /* +0x22 */ u16 numMaterial;
    /* +0x24 */ u32 texMtxMode;
    /* +0x28 */ u32 policy;
};

/* One animated material's record in a texture-SRT resource: the offset (from the record, plus 4) of its material
 * name, the animated-texture and animated-indirect-matrix bit sets, then one record offset per set bit.
 * size: 0x10 (a lower bound: the offset array runs on) */
struct ResAnmTexSrtMatData {
    /* +0x00 */ s32 toResName;
    /* +0x04 */ u32 flags;
    /* +0x08 */ u32 indFlags;
    /* +0x0C */ s32 toResAnmTexSrtTexData[1];
};

/* The one-word handle on a texture-SRT animation resource.  size: 0x4 */
class ResAnmTexSrt {
public:
    u32 GetNumMaterial() const;
    void GetAnmResult(TexSrtAnmResult* pResult, u32 id, f32 frame) const;

    /* +0x0 */ ResAnmTexSrtData* mpData;
};

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
    AnmObjTexSrtOverride(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding, AnmObjTexSrtRes** ppChildrenBuf,
                         int numChildren);
    static AnmObjTexSrtOverride* Construct(MEMAllocator* pHeap, u32* pSize, ResMdl mdl, int numChildren);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual ~AnmObjTexSrtOverride();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual const TexSrtAnmResult* GetResult(TexSrtAnmResult* pResult, u32 idx);

    static const TypeObj GetTypeObjStatic();
};

/* A texture-SRT animation played from a resource: its frame counter, the animation resource and the result cache.
 * size: 0x34 */
class AnmObjTexSrtRes : public AnmObjTexSrt, protected FrameCtrl {
public:
    AnmObjTexSrtRes(MEMAllocator* pHeap, ResAnmTexSrt res, u16* pBindingBuf, int numBinding,
                    TexSrtAnmResult* pCacheBuf);
    static AnmObjTexSrtRes* Construct(MEMAllocator* pHeap, u32* pSize, ResAnmTexSrt res, ResMdl mdl, bool bHasCache);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual ~AnmObjTexSrtRes();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame);
    virtual f32 GetFrame() const;
    virtual void UpdateFrame();
    virtual void SetUpdateRate(f32 rate);
    virtual f32 GetUpdateRate() const;
    virtual bool Bind(ResMdl mdl);

    virtual const TexSrtAnmResult* GetResult(TexSrtAnmResult* pResult, u32 idx);

    static const TypeObj GetTypeObjStatic();
    void UpdateCache();

    /* +0x18 */ /* FrameCtrl base (0x14 bytes) */
    /* +0x2C */ ResAnmTexSrt mRes;
    /* +0x30 */ TexSrtAnmResult* mpResultCache;
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_ANMTEXSRT_H */
