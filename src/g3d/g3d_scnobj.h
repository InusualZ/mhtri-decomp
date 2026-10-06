/* g3d/g3d_scnobj.h - nw4r g3d's scene objects: `ScnObj` (a node of the scene graph with its local, world and view
 *   matrices, bounding boxes, option flags and a user callback), `ScnLeaf` (a scaled leaf) and `ScnGroup` (a node
 *   holding a fixed-capacity child array), plus the callback and gather interfaces they call.
 *   Evidence: the three vtables of `g3d/g3d_scnobj.cpp` (.data 0x8058F488 ScnGroup, 0x8058F4C8 ScnLeaf,
 *   0x8058F4FC ScnObj) give the slot order (G3dObj's five, then ForEach, SetScnObjOption, GetScnObjOption,
 *   GetValueForSortOpa, GetValueForSortXlu, CalcWorldMtx, and ScnGroup's Insert and the two Removes); the ScnObj
 *   constructor 0x80081260 gives the layout (three MTX34 at +0x0C, two AABB at +0x9C, the flags at +0xCC, the two
 *   draw priorities 128 at +0xD0/+0xD1, the callback block at +0xD4..+0xDB) and the ScnGroup constructor 0x80082524
 *   the child array at +0xDC..+0xE7.  The member names are nw4r's.
 *   Each class declares its destructor (ScnLeaf: ForEach) before the inherited overrides: MWCC emits a vtable in
 *   the unit that defines the class's first declared out-of-line virtual, and IsDerivedFrom lives in
 *   `g3d/fn_80075DCC.cpp` while the vtables are this unit's.
 */
#ifndef MHTRI_G3D_G3D_SCNOBJ_H
#define MHTRI_G3D_G3D_SCNOBJ_H

#include "types.h"
#include "g3d/g3d_obj.h"     /* nw4r::g3d::G3dObj (rule 1) */
#include "nw4r/math.h"       /* nw4r::math::MTX34 / VEC3 */
#include "nw4r/fn_805012C4.h" /* nw4r::math::AABB / Frustum, owner nw4r/fn_805012C4.cpp (rule 2) */

namespace nw4r {
namespace g3d {

class ScnObj;

/* The user hook a scene object runs around its calc passes; its vtable lives with the classes that implement it.
 * size: 0x4 */
class IScnObjCallback {
public:
    virtual ~IScnObjCallback();
    /* untyped: caller-owned payload - the pass's info block */
    virtual void ExecCallback_CALC_WORLD(u32 timing, ScnObj* pObj, u32 param, void* pInfo);
    /* untyped: caller-owned payload - the pass's info block */
    virtual void ExecCallback_CALC_MAT(u32 timing, ScnObj* pObj, u32 param, void* pInfo);
    /* untyped: caller-owned payload - the pass's info block */
    virtual void ExecCallback_CALC_VIEW(u32 timing, ScnObj* pObj, u32 param, void* pInfo);
};

/* The collector the gather pass hands every scene object to (`ScnRoot`'s draw lists).  size: 0x4 */
class IScnObjGather {
public:
    /* How the collector culled the object: its children are visited for NOTCULLED and NOTEST. */
    enum CullingStatus {
        CULLINGSTATUS_NOTCULLED = 0,
        CULLINGSTATUS_NOTEST = 1,
        CULLINGSTATUS_CULLED = 2
    };

    virtual ~IScnObjGather();
    virtual CullingStatus Add(ScnObj* pObj, bool opa, bool xlu) = 0;
};

/* size: 0xDC */
class ScnObj : public G3dObj {
public:
    /* What a ForEach visitor tells the walk. */
    enum ForEachResult {
        FOREACH_RESULT_OK = 0,
        FOREACH_RESULT_RETURN = 1
    };
    /* untyped: caller-owned payload - the visitor's context */
    typedef ForEachResult (*ForEachFunc)(ScnObj* pObj, void* pInfo);

    /* The three matrices of mMtxArray. */
    enum ScnObjMtxType {
        MTX_LOCAL = 0,
        MTX_WORLD = 1,
        MTX_VIEW = 2,
        MTX_TYPE_MAX = 3
    };

    /* The two boxes of mAABB. */
    enum ScnObjBoundingVolumeType {
        BOUNDINGVOLUME_AABB_LOCAL = 0,
        BOUNDINGVOLUME_AABB_WORLD = 1,
        BOUNDINGVOLUME_MAX = 2
    };

    /* The bits of mScnObjFlags: one disable bit per G3dProc pass (bit task-1), then the culling and gather bits. */
    enum ScnObjFlag {
        SCNOBJFLAG_DISABLE_CALC_WORLD = 0x1,
        SCNOBJFLAG_DISABLE_CALC_MAT = 0x2,
        SCNOBJFLAG_DISABLE_CALC_VTX = 0x4,
        SCNOBJFLAG_DISABLE_CALC_VIEW = 0x8,
        SCNOBJFLAG_DISABLE_GATHER_SCNOBJ = 0x10,
        SCNOBJFLAG_DISABLE_DRAW_OPA = 0x20,
        SCNOBJFLAG_DISABLE_DRAW_XLU = 0x40,
        SCNOBJFLAG_DISABLE_UPDATEFRAME = 0x80,
        SCNOBJFLAG_ENABLE_CULLING = 0x10000000,
        SCNOBJFLAG_NOT_GATHER_DRAW_OPA = 0x20000000,
        SCNOBJFLAG_NOT_GATHER_DRAW_XLU = 0x40000000,
        SCNOBJFLAG_MTX_LOCAL_IDENTITY = 0x80000000
    };

    /* The option ids SetScnObjOption / GetScnObjOption take. */
    enum ScnObjOption {
        OPTION_NONE = 0,
        OPTION_DISABLE_GATHER_SCNOBJ = 1,
        OPTION_DISABLE_CALC_WORLD = 2,
        OPTION_DISABLE_CALC_MAT = 3,
        OPTION_DISABLE_CALC_VTX = 4,
        OPTION_DISABLE_CALC_VIEW = 5,
        OPTION_DISABLE_DRAW_OPA = 6,
        OPTION_DISABLE_DRAW_XLU = 7,
        OPTION_DISABLE_UPDATEFRAME = 8,
        OPTION_ENABLE_CULLING = 9
    };

    /* When a callback runs relative to the pass's own work. */
    enum Timing {
        CALLBACK_TIMING_A = 1,
        CALLBACK_TIMING_B = 2,
        CALLBACK_TIMING_C = 4
    };

    /* The passes mCallbackExecOpMask enables the callback for. */
    enum ExecOp {
        EXECOP_CALC_WORLD = 1,
        EXECOP_CALC_MAT = 2,
        EXECOP_CALC_VIEW = 4
    };

    explicit ScnObj(MEMAllocator* pHeap);

    virtual ~ScnObj();
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;
    /* untyped: caller-owned payload - the visitor's context */
    virtual ForEachResult ForEach(ForEachFunc pFunc, void* pInfo, bool postOrder) = 0;
    virtual bool SetScnObjOption(u32 option, u32 value);
    virtual bool GetScnObjOption(u32 option, u32* pValue) const;
    virtual f32 GetValueForSortOpa() const;
    virtual f32 GetValueForSortXlu() const;
    virtual void CalcWorldMtx(const math::MTX34* pParent, u32* pParam);

    bool SetMtx(ScnObjMtxType type, const math::MTX34* pMtx);
    bool GetMtx(ScnObjMtxType type, math::MTX34* pMtx) const;
    math::MTX34* GetMtxPtr(ScnObjMtxType type);
    bool SetBoundingVolume(ScnObjBoundingVolumeType type, const math::AABB* pAABB);
    bool GetBoundingVolume(ScnObjBoundingVolumeType type, math::AABB* pAABB) const;
    void CalcViewMtx(const math::MTX34* pCamera);
    u32 TestScnObjFlag(ScnObjFlag flag) const;
    void SetScnObjFlag(ScnObjFlag flag, u32 on);
    bool IsG3dProcDisabled(u32 task) const;
    /* untyped: caller-owned payload - the pass's info block */
    void CheckCallback_CALC_WORLD(Timing timing, u32 param, void* pInfo);
    /* untyped: caller-owned payload - the pass's info block */
    void CheckCallback_CALC_MAT(Timing timing, u32 param, void* pInfo);
    /* untyped: caller-owned payload - the pass's info block */
    void CheckCallback_CALC_VIEW(Timing timing, u32 param, void* pInfo);
    static const TypeObj GetTypeObjStatic();

    /* +0x0C */ math::MTX34 mMtxArray[MTX_TYPE_MAX];
    /* +0x9C */ math::AABB mAABB[BOUNDINGVOLUME_MAX];
    /* +0xCC */ u32 mScnObjFlags;
    /* +0xD0 */ u8 mPriorityDrawOpa;
    /* +0xD1 */ u8 mPriorityDrawXlu;
    /* +0xD2 */ u8 pad_0xD2;
    /* +0xD3 */ u8 pad_0xD3;
    /* +0xD4 */ IScnObjCallback* mpFnCallback;
    /* +0xD8 */ u8 mCallbackTiming;
    /* +0xD9 */ u8 mCallbackDeleteOption;
    /* +0xDA */ u16 mCallbackExecOpMask;
};

/* size: 0xE8 */
class ScnLeaf : public ScnObj {
public:
    /* How mScale scales the leaf. */
    enum ScaleProperty {
        NOT_SCALED = 0,
        UNIFORM_SCALED = 1,
        NONUNIFORM_SCALED = 2
    };

    explicit ScnLeaf(MEMAllocator* pHeap);

    /* untyped: caller-owned payload - the visitor's context */
    virtual ForEachResult ForEach(ForEachFunc pFunc, void* pInfo, bool postOrder);
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo) = 0; /* untyped: caller-owned payload */
    virtual ~ScnLeaf();
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;
    virtual bool SetScnObjOption(u32 option, u32 value);
    virtual bool GetScnObjOption(u32 option, u32* pValue) const;
    virtual void CalcWorldMtx(const math::MTX34* pParent, u32* pParam);

    ScaleProperty GetScaleProperty() const;
    /* untyped: caller-owned payload - the pass's info block */
    void DefG3dProcScnLeaf(u32 task, u32 param, void* pInfo);
    static const TypeObj GetTypeObjStatic();

    /* +0xDC */ math::VEC3 mScale;
};

/* size: 0xE8 */
class ScnGroup : public ScnObj {
public:
    ScnGroup(MEMAllocator* pHeap, ScnObj** ppObjArray, u32 capacity);

    virtual ~ScnGroup();
    virtual bool IsDerivedFrom(TypeObj type) const;
    virtual void G3dProc(u32 task, u32 param, void* pInfo); /* untyped: caller-owned payload */
    virtual const TypeObj GetTypeObj() const;
    virtual const char* GetTypeName() const;
    /* untyped: caller-owned payload - the visitor's context */
    virtual ForEachResult ForEach(ForEachFunc pFunc, void* pInfo, bool postOrder);
    virtual bool Insert(u32 idx, ScnObj* pObj);
    virtual ScnObj* Remove(u32 idx);
    virtual bool Remove(ScnObj* pObj);

    u32 Size() const;
    bool Empty() const;
    ScnObj* PopBack();
    void Clear();
    /* untyped: caller-owned payload - the pass's info block */
    void DefG3dProcScnGroup(u32 task, u32 param, void* pInfo);
    void G3dProcGatherScnObj(u32 param, IScnObjGather* pGather);
    void G3dProcCalcWorld(u32 param, const math::MTX34* pParent);
    /* untyped: caller-owned payload - the pass's info block */
    void G3dProcCalcMat(u32 param, void* pInfo);
    void G3dProcCalcView(u32 param, const math::MTX34* pCamera);
    static const TypeObj GetTypeObjStatic();

    /* +0xDC */ ScnObj** mpScnObjArray;
    /* +0xE0 */ u32 mSizeScnObj;
    /* +0xE4 */ u32 mNumScnObj;
};

}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_G3D_G3D_SCNOBJ_H */
