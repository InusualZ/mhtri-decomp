/* g3d/g3d_anmchr.h - the cross-unit declarations of `g3d/g3d_anmchr.cpp` (C linkage, plain map stems).
 *   0x8005DC60/0x8005DCD0 store through their first argument and return it; 0x800628A4 and 0x8005DC24 load through
 *   theirs.  The owner's own forward prototypes are ABI-identical. */
#ifndef MHTRI_G3D_G3D_ANMCHR_H
#define MHTRI_G3D_G3D_ANMCHR_H

#include "types.h"
#include "g3d/g3d_obj.h" /* nw4r::g3d::G3dObj (rule 1) */

#ifdef __cplusplus
extern "C" {
#endif

void **fn_8005DC60(void **out, void *v);   /* 0x8005DC60 - stores `v` through `out`, returns `out` */
void **fn_8005DCD0(void **out, void *v);   /* 0x8005DCD0 - stores `v` through `out`, returns `out` */
u32 fn_800628B4(void *self);               /* 0x800628B4 - `*(u32*)self != 0` */
void *fn_800628A4(void *self);             /* 0x800628A4 - loads the word at +0x0 of `self` */


/* The name-record store helper (0x800638B8) `g3d/fn_800680CC.cpp` and `g3d/g3d_scnmdl.cpp` also call. */
const u8 **type_obj_set_name(const u8 **out, const u8 *v); /* stores `v` through `out` and returns `out` */



/* The frame/rate helpers `g3d/g3d_resanmchr.cpp`'s channel evaluators call (types the target bodies imply). */
f32 math_reciprocal(f32 value);                /* 0x800610AC - the reciprocal helper */
void *fn_800618BC(void *self);             /* 0x800618BC - the resource-table base */
s32 fn_800628C8(void *self, s32 key);      /* 0x800628C8 - the table entry lookup */


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
struct ChrAnmResult;

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

    static const TypeObj GetTypeObjStatic();

    /* +0x20 */ f32* mpWeightArray;
};

/* A character animation played from a resource: its frame counter, the animation resource and the result cache.
 * The members are declared as they are written.  size: 0x34 */
class AnmObjChrRes : public AnmObjChr {
public:
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual ~AnmObjChrRes();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;

    static const TypeObj GetTypeObjStatic();

    /* +0x18 */ FrameCtrl mFrameCtrl;
    /* +0x2C */ void* mpRes;               /* untyped: opaque handle - the ResAnmChr block */
    /* +0x30 */ ChrAnmResult* mpResultCache;
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_ANMCHR_H */

