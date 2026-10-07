/* g3d/g3d_anmchr.h - the cross-unit declarations of `g3d/g3d_anmchr.cpp` (C linkage, plain map stems).
 *   0x8005DC60/0x8005DCD0 store through their first argument and return it; 0x800628A4 and 0x8005DC24 load through
 *   theirs.  The owner's own forward prototypes are ABI-identical. */
#ifndef MHTRI_G3D_G3D_ANMCHR_H
#define MHTRI_G3D_G3D_ANMCHR_H

#include "types.h"
#include "g3d/g3d_obj.h" /* nw4r::g3d::G3dObj (rule 1) */
#include "g3d/g3d_rescommon.h" /* nw4r::g3d::ResCommon (rule 1) */
#include "nw4r/math.h" /* nw4r::math::VEC3, MTX34, QUAT (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

void **fn_8005DC60(void **out, void *v);   /* 0x8005DC60 - stores `v` through `out`, returns `out` */
void **type_obj_set_name_anmchr(void **out, void *v);   /* 0x8005DCD0 - stores `v` through `out`, returns `out` */
u32 res_dic_is_valid(void *self);               /* 0x800628B4 - `*(u32*)self != 0` */
void *res_dic_ptr(void *self);             /* 0x800628A4 - loads the word at +0x0 of `self` */


/* The animation type-name records (`.rodata`: a length word, then the NUL-terminated name) this unit's type-info
 * members read; they sit in this unit's range (0x8056F500-0x8056F550). */
extern u8 anm_typename_AnmObjChrNode[];   /* 0x8056F510 - "AnmObjChrNode" */
extern u8 anm_typename_AnmObjChrBlend[];  /* 0x8056F524 - "AnmObjChrBlend" */

/* The name-record store helper (0x800638B8) `g3d/fn_800680CC.cpp` and `g3d/g3d_scnmdl.cpp` also call. */
const u8 **type_obj_set_name(const u8 **out, const u8 *v); /* stores `v` through `out` and returns `out` */



/* The frame/rate helpers `g3d/g3d_resanmchr.cpp`'s channel evaluators call (types the target bodies imply). */
f32 math_reciprocal(f32 value);                /* 0x800610AC - the reciprocal helper */



#ifdef __cplusplus
}
#endif

/* The object/vtable pair the dispatch wrappers of this unit, `g3d/fn_80063888.cpp` and `g3d/fn_800680CC.cpp` share (rule 1). */
typedef u32 (*G3dVtMethod)(void *);
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ G3dVtMethod method_0x14;
} G3dVtbl; /* size: 0x18 */
typedef struct {
    /* +0x00 */ G3dVtbl *vt;
} G3dObj; /* size: 0x4 */

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class AnmObjChrRes;

/* One node's evaluated character-animation result: the flag word (0: nothing animated) and the transform the
 * blend and the world pass read.  size: 0x4C (the cache's per-node stride) */
struct ChrAnmResult {
    enum {
        FLAG_ANM_EXISTS = (1 << 0),
        FLAG_SCALE_ONE = (1 << 3),
        FLAG_ROT_ZERO = (1 << 5),
        FLAG_TRANS_ZERO = (1 << 6),
        FLAG_SSC_APPLY = (1 << 7),
        FLAG_SSC_PARENT = (1 << 8),
        FLAG_XSI_SCALING = (1 << 9)
    };

    /* +0x00 */ u32 flags;
    /* +0x04 */ math::VEC3 s;
    /* +0x10 */ math::VEC3 rawR;
    /* +0x1C */ math::MTX34 rt;
};

/* The animation's play policy: what a frame past the end folds back to. */
enum AnmPolicy {
    ANM_POLICY_ONETIME = 0,
    ANM_POLICY_LOOP = 1
};

/* 0x80061C70 (0x74): the frame-folding function for `policy`. */
PlayPolicyFunc GetAnmPlayPolicy(AnmPolicy policy);

/* A character-animation resource block: its revision, the node dictionary, the frame and node counts and the
 * play policy.  size: 0x28 (approximation: only the read fields are named) */
struct ResAnmChrData {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u32 revision;
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ s32 toChrDataDic;
    /* +0x14 */ u8 pad_0x14[0xC];
    /* +0x20 */ u16 numFrame;
    /* +0x22 */ u16 numNode;
    /* +0x24 */ AnmPolicy policy;
};

/* One animated node's record in a character-animation resource: the offset (from the record, plus 4) of its
 * node name, then the option/ScaleType flags.  size: 0x8 (a lower bound: only these two words are evidenced) */
struct ResAnmChrNodeData {
    /* +0x00 */ s32 toResName;
    /* +0x04 */ u32 flags;
};

/* The one-word handle on a character-animation resource.  size: 0x4 */
class ResAnmChr : public ResCommon<ResAnmChrData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResAnmChr)
    ResAnmChr() {}

    u32 GetNumNode() const;
    AnmPolicy GetAnmPolicy() const;
    int GetNumFrame() const;
    void GetAnmResult(ChrAnmResult* pResult, u32 id, f32 frame) const;
};

/* The character animation interface: one binding word per model node (bit 15: no animation, bit 14: undefined)
 * and the attach/weight protocol the blend and node classes implement.  size: 0x18 */
class AnmObjChr : public AnmObj {
public:
    enum BindOption {
        BIND_ONE,
        BIND_PARTIAL
    };

    AnmObjChr(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual ~AnmObjChr();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame) = 0;
    virtual f32 GetFrame() const = 0;
    virtual void UpdateFrame() = 0;
    virtual void SetUpdateRate(f32 rate) = 0;
    virtual f32 GetUpdateRate() const = 0;
    virtual bool Bind(ResMdl mdl) = 0;
    virtual void Release();

    virtual const ChrAnmResult* GetResult(ChrAnmResult* pResult, u32 idx) = 0;
    virtual AnmObjChrRes* Attach(int idx, AnmObjChrRes* pRes);
    virtual AnmObjChrRes* Detach(int idx);
    virtual void DetachAll();
    virtual void SetWeight(int idx, f32 weight);
    virtual f32 GetWeight(int idx) const;
    virtual bool Bind(ResMdl mdl, u32 target, BindOption option) = 0;
    virtual void Release(ResMdl mdl, u32 target, BindOption option) = 0;

    bool TestExistence(u32 idx) const;
    bool TestDefined(u32 idx) const;
    static const TypeObj GetTypeObjStatic();

    /* +0x10 */ int mNumBinding;
    /* +0x14 */ u16* mpBinding;
};

/* A character animation made of child animations: the child array and the attach/detach bookkeeping the blend
 * class builds on.  size: 0x20 */
class AnmObjChrNode : public AnmObjChr {
public:
    AnmObjChrNode(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding, AnmObjChrRes** ppChildrenBuf,
                  int numChildren);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual ~AnmObjChrNode();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame);
    virtual f32 GetFrame() const;
    virtual void UpdateFrame();
    virtual void SetUpdateRate(f32 rate);
    virtual f32 GetUpdateRate() const;
    virtual bool Bind(ResMdl mdl);
    virtual void Release();

    virtual const ChrAnmResult* GetResult(ChrAnmResult* pResult, u32 idx) = 0;
    virtual AnmObjChrRes* Attach(int idx, AnmObjChrRes* pRes);
    virtual AnmObjChrRes* Detach(int idx);
    virtual void DetachAll();
    virtual bool Bind(ResMdl mdl, u32 target, BindOption option);
    virtual void Release(ResMdl mdl, u32 target, BindOption option);

    static const TypeObj GetTypeObjStatic();

    /* +0x18 */ int mChildrenArraySize;
    /* +0x1C */ AnmObjChrRes** mpChildrenArray;
};

/* A node that blends its children's results by weight.  size: 0x24 */
class AnmObjChrBlend : public AnmObjChrNode {
public:
    AnmObjChrBlend(MEMAllocator* pHeap, u16* pBindingBuf, int numBinding, AnmObjChrRes** ppChildrenBuf,
                   int numChildren, f32* pWeightBuf);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual ~AnmObjChrBlend();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual const ChrAnmResult* GetResult(ChrAnmResult* pResult, u32 idx);
    virtual void SetWeight(int idx, f32 weight);
    virtual f32 GetWeight(int idx) const;

    static AnmObjChrBlend* Construct(MEMAllocator* pHeap, u32* pSize, ResMdl mdl, int numChildren);
    static const TypeObj GetTypeObjStatic();

    /* +0x20 */ f32* mpWeightArray;
};

/* A character animation played from a resource: its frame counter, the animation resource and the result cache.
 * The members are declared as they are written.  size: 0x34 */
class AnmObjChrRes : public AnmObjChr, protected FrameCtrl {
public:
    AnmObjChrRes(MEMAllocator* pHeap, ResAnmChr res, u16* pBindingBuf, int numBinding, ChrAnmResult* pCacheBuf);
    static AnmObjChrRes* Construct(MEMAllocator* pHeap, u32* pSize, ResAnmChr res, ResMdl mdl, bool bHasCache);

    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual ~AnmObjChrRes();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    virtual void SetFrame(f32 frame);
    virtual f32 GetFrame() const;
    virtual void UpdateFrame();
    virtual void SetUpdateRate(f32 rate);
    virtual f32 GetUpdateRate() const;
    virtual bool Bind(ResMdl mdl);
    using AnmObjChr::Release;

    virtual const ChrAnmResult* GetResult(ChrAnmResult* pResult, u32 idx);
    virtual bool Bind(ResMdl mdl, u32 target, BindOption option);
    virtual void Release(ResMdl mdl, u32 target, BindOption option);

    static const TypeObj GetTypeObjStatic();
    void UpdateCache();

    /* +0x18 */ /* FrameCtrl base (0x14 bytes) */
    /* +0x2C */ ResAnmChr mRes;
    /* +0x30 */ ChrAnmResult* mpResultCache;
};

}  // namespace g3d
}  // namespace nw4r

/* 0x800628C8 (0x4C) - the dictionary `key` bytes into the animation resource, as the handle word (0 for key 0). */
extern "C" s32 res_anm_chr_ofs_to_dic(const nw4r::g3d::ResAnmChr* pSelf, s32 key);
#endif

#endif /* MHTRI_G3D_G3D_ANMCHR_H */

