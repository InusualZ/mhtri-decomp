/*
 * g3d/g3d_scnobj.cpp - nw4r g3d `ScnObj` base object and its `ScnLeaf`/`ScnGroup` subclasses.
 * RANGE. .text 0x800813B8-0x800827E4 (36 functions); extab, extabindex, .data 0x8058F3D8-0x8058F530 (opens on
 *   "g3d_scnobj.cpp"; the `!GetParent()` assert and jumptable_8058F40C/jumptable_8058F434), .sdata2
 *   0x80795E64-0x80795E68.  Both edges are tudiscover's weak cuts and `.data` fragment boundaries; the
 *   0x80082668-0x800827E4 tail is here because the run-time type members read the ScnObj/ScnLeaf/ScnGroup
 *   name records (scn_typename_ScnObj/_ScnLeaf/_ScnGroup).
 * NAMES. The members of ScnObj, ScnLeaf, ScnGroup, IScnObjCallback and IScnObjGather are nw4r's: the three vtables
 *   give the slot order, the asserts name the file, and the map rows carry the compiler's manglings.
 *   scn_typename_ScnObj/_ScnLeaf/_ScnGroup are GUESSES (the `.rodata` records by their strings); scnobj_culling_frustum
 *   is a GUESS (the `.sbss` word the gather pass saves, clears and restores around a NOTEST subtree and
 *   `g3d/g3d_scnroot.cpp`'s gather intersects boxes against); type_obj_set_name_scnleaf and
 *   type_obj_set_name_scngroup are GUESSES (the type-name store copies of `g3d/fn_80075DCC.cpp` the ScnLeaf and
 *   ScnGroup type members call); G3dProcGatherScnObj, G3dProcCalcWorld, G3dProcCalcMat and G3dProcCalcView are
 *   GUESSES (the per-pass ScnGroup members DefG3dProcScnGroup dispatches to).
 * RESIDUALS. Unwritten: 0x8008147C-0x800814C0 (IScnObjCallback's destructor: defining it would emit the interface's
 *   vtable here, which the target object does not carry) and 0x80081804-0x80081838 (the AABB copy assignment
 *   retail emits out of line; `nw4r::math::AABB` is a plain struct, so MWCC copies inline).
 *   Partial: SetBoundingVolume and GetBoundingVolume (retail calls the out-of-line AABB copy `fn_80081804`; ours
 *   copies inline), Remove(u32) (retail keeps `idx * 4` in
 *   a saved register across the detach call), Insert (one `subf` scheduled later), DefG3dProcScnGroup (retail keeps
 *   a dead `b` after the CALC_VIEW case), and the rows whose only difference is a string relocation name.
 *   The ScnObj constructor (0x80081260), CalcWorldMtx (0x80081188) and CalcViewMtx (0x80081250) sit in
 *   `g3d/g3d_scnmdlsmpl.cpp`'s range and IsDerivedFrom, GetTypeObjStatic, TestScnObjFlag, SetScnObjFlag and the
 *   CheckCallback members in `g3d/fn_80075DCC.cpp`'s: the left seam is unmoved (nw4r-l1#110).  The three 0x10-byte
 *   name records at `.rodata` 0x8056F6A0 and scnobj_culling_frustum (`.sbss` 0x807948E8, defined here) are unclaimed
 *   (nw4r-l1#111), so the object's `.sbss` has no claim to land in yet.
 * SHAPES. `(int)mCallbackDeleteOption == 1` and `(u32)type < MTX_TYPE_MAX` reproduce retail's signed and unsigned
 *   compares; the destructors and CalcWorldMtx keep `#pragma peephole off` (retail's `extsh` and `clrlwi` + `cmpwi`).
 */

#include "types.h"
#include "g3d/g3d_scnobj.h"   /* this unit's own classes (rule 1) */
#include "g3d/g3d_anmchr.h"   /* type_obj_set_name, owner g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/fn_80075DCC.h"  /* the type-name store copies, owner g3d/fn_80075DCC.cpp (rule 2) */
#include "g3d/g3d_calcview.h" /* mtx34_copy_ps, owner g3d/g3d_calcview.cpp (rule 2) */
#include "fn_8004CAD8.h"        /* mtx34_identity, owner fn_8004CAD8.cpp (rule 2) */
#include "unsplit/g3d.h"      /* the name records and the culling frustum no unit owns (rule 2) */
#include "nw4r/db_assert.h"   /* nw4r::db::Panic (rule 2) */
#include "MSL/algorithm.h"    /* std::find / std::distance */

#pragma pool_data off

/* The frustum the gather pass culls against while it is set (`ScnRoot`'s camera); a NOTEST status clears it for the
 * subtree.  `.sbss` 0x807948E8, between g3d_scnmdlsmpl's and g3d_scnroot's words in link order. */
const nw4r::math::Frustum* scnobj_culling_frustum;

/* ------------------------------------------------------------------------------------------------ */
/* ScnObj                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* 0x800813B8 (0xC4): asserts the object is detached and deletes an owned callback. */
#pragma peephole off
nw4r::g3d::ScnObj::~ScnObj()
{
    if (GetParent()) {
        nw4r::db::Panic("g3d_scnobj.cpp", 0x61, "NW4R:Failed assertion !GetParent()");
    }
    if (mpFnCallback != NULL && (int)mCallbackDeleteOption == 1) {
        delete mpFnCallback;
    }
}
#pragma peephole on

/* 0x800814C0 (0xB4): sets the flag an option id names; false for an unknown id. */
bool nw4r::g3d::ScnObj::SetScnObjOption(u32 option, u32 value)
{
    switch (option) {
    case OPTION_DISABLE_GATHER_SCNOBJ:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_GATHER_SCNOBJ, value);
        break;
    case OPTION_DISABLE_CALC_WORLD:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_CALC_WORLD, value);
        break;
    case OPTION_DISABLE_CALC_MAT:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_CALC_MAT, value);
        break;
    case OPTION_DISABLE_CALC_VTX:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_CALC_VTX, value);
        break;
    case OPTION_DISABLE_CALC_VIEW:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_CALC_VIEW, value);
        break;
    case OPTION_DISABLE_DRAW_OPA:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_DRAW_OPA, value);
        break;
    case OPTION_DISABLE_DRAW_XLU:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_DRAW_XLU, value);
        break;
    case OPTION_DISABLE_UPDATEFRAME:
        SetScnObjFlag(SCNOBJFLAG_DISABLE_UPDATEFRAME, value);
        break;
    case OPTION_ENABLE_CULLING:
        SetScnObjFlag(SCNOBJFLAG_ENABLE_CULLING, value);
        break;
    default:
        return false;
    }
    return true;
}

/* 0x80081574 (0xF4): reads the flag an option id names; false for an unknown id or no output. */
bool nw4r::g3d::ScnObj::GetScnObjOption(u32 option, u32* pValue) const
{
    if (pValue == NULL) {
        return false;
    }
    switch (option) {
    case OPTION_DISABLE_GATHER_SCNOBJ:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_GATHER_SCNOBJ);
        break;
    case OPTION_DISABLE_CALC_WORLD:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_CALC_WORLD);
        break;
    case OPTION_DISABLE_CALC_MAT:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_CALC_MAT);
        break;
    case OPTION_DISABLE_CALC_VTX:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_CALC_VTX);
        break;
    case OPTION_DISABLE_CALC_VIEW:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_CALC_VIEW);
        break;
    case OPTION_DISABLE_DRAW_OPA:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_DRAW_OPA);
        break;
    case OPTION_DISABLE_DRAW_XLU:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_DRAW_XLU);
        break;
    case OPTION_DISABLE_UPDATEFRAME:
        *pValue = TestScnObjFlag(SCNOBJFLAG_DISABLE_UPDATEFRAME);
        break;
    case OPTION_ENABLE_CULLING:
        *pValue = TestScnObjFlag(SCNOBJFLAG_ENABLE_CULLING);
        break;
    default:
        return false;
    }
    return true;
}

/* 0x80081668 (0xAC): sets a matrix, or resets it to identity for NULL (the local one also tracks the identity
 * flag). */
bool nw4r::g3d::ScnObj::SetMtx(ScnObjMtxType type, const math::MTX34* pMtx)
{
    if ((u32)type < MTX_TYPE_MAX) {
        if (pMtx != NULL) {
            if (type == MTX_LOCAL) {
                SetScnObjFlag(SCNOBJFLAG_MTX_LOCAL_IDENTITY, false);
            }
            mtx34_copy_ps(&mMtxArray[type], pMtx);
        } else {
            if (type == MTX_LOCAL) {
                SetScnObjFlag(SCNOBJFLAG_MTX_LOCAL_IDENTITY, true);
            }
            mtx34_identity(&mMtxArray[type]);
        }
        return true;
    }
    return false;
}

/* 0x80081714 (0x50): copies a matrix out. */
bool nw4r::g3d::ScnObj::GetMtx(ScnObjMtxType type, math::MTX34* pMtx) const
{
    if (pMtx != NULL && (u32)type < MTX_TYPE_MAX) {
        mtx34_copy_ps(pMtx, &mMtxArray[type]);
        return true;
    }
    return false;
}

/* 0x80081764 (0xC): the opaque sort key: the negated view-space depth. */
f32 nw4r::g3d::ScnObj::GetValueForSortOpa() const
{
    return -mMtxArray[MTX_VIEW].m[2][3];
}

/* 0x80081770 (0x8): the translucent sort key: the view-space depth. */
f32 nw4r::g3d::ScnObj::GetValueForSortXlu() const
{
    return mMtxArray[MTX_VIEW].m[2][3];
}

/* 0x80081778 (0x8C): sets a bounding box and enables culling, or disables culling for NULL. */
bool nw4r::g3d::ScnObj::SetBoundingVolume(ScnObjBoundingVolumeType type, const math::AABB* pAABB)
{
    if (pAABB != NULL) {
        if (type < BOUNDINGVOLUME_MAX) {
            mAABB[type] = *pAABB;
            return SetScnObjOption(OPTION_ENABLE_CULLING, true);
        }
        return false;
    }
    return SetScnObjOption(OPTION_ENABLE_CULLING, false);
}

/* 0x80081838 (0x58): copies a bounding box out. */
bool nw4r::g3d::ScnObj::GetBoundingVolume(ScnObjBoundingVolumeType type, math::AABB* pAABB) const
{
    if (pAABB != NULL) {
        if (type < BOUNDINGVOLUME_MAX) {
            *pAABB = mAABB[type];
            return true;
        }
        return false;
    }
    return false;
}

/* ------------------------------------------------------------------------------------------------ */
/* ScnLeaf                                                                                          */
/* ------------------------------------------------------------------------------------------------ */

/* 0x80081890 (0x38): visits the leaf. */
/* untyped: caller-owned payload - the visitor's context */
nw4r::g3d::ScnObj::ForEachResult nw4r::g3d::ScnLeaf::ForEach(ForEachFunc pFunc, void* pInfo, bool postOrder)
{
    return (ForEachResult)(pFunc(this, pInfo) == FOREACH_RESULT_RETURN);
}

/* 0x800818C8 (0x40): the draw-disable option sets both draw flags; the rest are the base's. */
bool nw4r::g3d::ScnLeaf::SetScnObjOption(u32 option, u32 value)
{
    if (option == 0x10001) {
        SetScnObjFlag((ScnObjFlag)(SCNOBJFLAG_DISABLE_DRAW_OPA | SCNOBJFLAG_DISABLE_DRAW_XLU), value);
    } else {
        return ScnObj::SetScnObjOption(option, value);
    }
    return true;
}

/* 0x80081908 (0x60): the draw-disable option reads both draw flags; the rest are the base's. */
bool nw4r::g3d::ScnLeaf::GetScnObjOption(u32 option, u32* pValue) const
{
    if (pValue == NULL) {
        return false;
    }
    if (option == 0x10001) {
        *pValue = TestScnObjFlag((ScnObjFlag)(SCNOBJFLAG_DISABLE_DRAW_OPA | SCNOBJFLAG_DISABLE_DRAW_XLU));
    } else {
        return ScnObj::GetScnObjOption(option, pValue);
    }
    return true;
}

/* 0x80081968 (0x88): computes the world matrix with the leaf's scale and its world box; bit 0 of the parameter
 * skips one pass. */
#pragma peephole off
void nw4r::g3d::ScnLeaf::CalcWorldMtx(const math::MTX34* pParent, u32* pParam)
{
    if (pParam != NULL && (*pParam & 1)) {
        *pParam &= ~1;
        return;
    }
    ScnObj::CalcWorldMtx(pParent, pParam);
    math::MTX34Scale(&mMtxArray[MTX_WORLD], &mMtxArray[MTX_WORLD], &mScale);
    if (TestScnObjFlag(SCNOBJFLAG_ENABLE_CULLING)) {
        mAABB[BOUNDINGVOLUME_AABB_WORLD].Set(&mAABB[BOUNDINGVOLUME_AABB_LOCAL], &mMtxArray[MTX_WORLD]);
    }
}
#pragma peephole on

/* 0x800819F0 (0x40): classifies the scale as none, uniform or non-uniform. */
nw4r::g3d::ScnLeaf::ScaleProperty nw4r::g3d::ScnLeaf::GetScaleProperty() const
{
    if (mScale.x == mScale.y && mScale.y == mScale.z) {
        if (1.0f == mScale.x) {
            return NOT_SCALED;
        }
        return UNIFORM_SCALED;
    }
    return NONUNIFORM_SCALED;
}

/* 0x80081A30 (0x158): a leaf's G3dProc work: the calc passes with their callbacks and the parent links. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnLeaf::DefG3dProcScnLeaf(u32 task, u32 param, void* pInfo)
{
    switch (task) {
    case G3DPROC_CALC_WORLD:
        CheckCallback_CALC_WORLD(CALLBACK_TIMING_A, param, pInfo);
        CalcWorldMtx(static_cast<const math::MTX34*>(pInfo), &param);
        CheckCallback_CALC_WORLD(CALLBACK_TIMING_B, param, pInfo);
        CheckCallback_CALC_WORLD(CALLBACK_TIMING_C, param, pInfo);
        break;
    case G3DPROC_CALC_MAT:
        CheckCallback_CALC_MAT(CALLBACK_TIMING_A, param, pInfo);
        CheckCallback_CALC_MAT(CALLBACK_TIMING_C, param, pInfo);
        break;
    case G3DPROC_CALC_VIEW:
        CheckCallback_CALC_VIEW(CALLBACK_TIMING_A, param, pInfo);
        CalcViewMtx(static_cast<const math::MTX34*>(pInfo));
        CheckCallback_CALC_VIEW(CALLBACK_TIMING_B, param, pInfo);
        CheckCallback_CALC_VIEW(CALLBACK_TIMING_C, param, pInfo);
        break;
    case G3DPROC_DETACH_PARENT:
        SetParent(NULL);
        break;
    case G3DPROC_ATTACH_PARENT:
        if (GetParent()) {
            nw4r::db::Panic("g3d_scnobj.cpp", 0x1EC, "NW4R:Failed assertion !GetParent()");
        }
        SetParent(static_cast<G3dObj*>(pInfo));
        break;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* ScnGroup                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* 0x80081B88 (0x140): visits the group and its children, pre- or post-order; stops on RETURN. */
/* untyped: caller-owned payload - the visitor's context */
nw4r::g3d::ScnObj::ForEachResult nw4r::g3d::ScnGroup::ForEach(ForEachFunc pFunc, void* pInfo, bool postOrder)
{
    if (postOrder) {
        for (u32 i = 0; i < Size(); i++) {
            if (mpScnObjArray[i]->ForEach(pFunc, pInfo, false) == FOREACH_RESULT_RETURN) {
                return FOREACH_RESULT_RETURN;
            }
        }
        return (ForEachResult)(pFunc(this, pInfo) == FOREACH_RESULT_RETURN);
    } else {
        ForEachResult result = pFunc(this, pInfo);
        if (result == FOREACH_RESULT_OK) {
            for (u32 i = 0; i < Size(); i++) {
                if (mpScnObjArray[i]->ForEach(pFunc, pInfo, false) == FOREACH_RESULT_RETURN) {
                    return FOREACH_RESULT_RETURN;
                }
            }
            return FOREACH_RESULT_OK;
        }
        return (ForEachResult)(result == FOREACH_RESULT_RETURN);
    }
}

/* 0x80081CC8 (0x124): hands the group to the collector and gathers the children it did not cull; a NOTEST
 * status turns culling off for the subtree. */
void nw4r::g3d::ScnGroup::G3dProcGatherScnObj(u32 param, IScnObjGather* pGather)
{
    IScnObjGather::CullingStatus status = pGather->Add(this, !TestScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_OPA),
                                                       !TestScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_XLU));
    if (status == IScnObjGather::CULLINGSTATUS_NOTCULLED) {
        for (u32 i = 0; i < mNumScnObj; i++) {
            mpScnObjArray[i]->G3dProc(G3DPROC_GATHER_SCNOBJ, param, pGather);
        }
    } else if (status == IScnObjGather::CULLINGSTATUS_NOTEST) {
        const math::Frustum* pFrustum = scnobj_culling_frustum;
        scnobj_culling_frustum = NULL;
        for (u32 i = 0; i < mNumScnObj; i++) {
            mpScnObjArray[i]->G3dProc(G3DPROC_GATHER_SCNOBJ, param, pGather);
        }
        scnobj_culling_frustum = pFrustum;
    }
}

/* 0x80081DEC (0xE4): computes the group's world matrix and passes it to the children. */
void nw4r::g3d::ScnGroup::G3dProcCalcWorld(u32 param, const math::MTX34* pParent)
{
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_A, param, (void*)pParent);
    CalcWorldMtx(pParent, &param);
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_B, param, (void*)pParent);
    math::MTX34* pWorld = GetMtxPtr(MTX_WORLD);
    for (u32 i = 0; i < mNumScnObj; i++) {
        mpScnObjArray[i]->G3dProc(G3DPROC_CALC_WORLD, param, pWorld);
    }
    CheckCallback_CALC_WORLD(CALLBACK_TIMING_C, param, (void*)pParent);
}

/* 0x80081ED0 (0xA0): passes the material pass to the children. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnGroup::G3dProcCalcMat(u32 param, void* pInfo)
{
    CheckCallback_CALC_MAT(CALLBACK_TIMING_A, param, pInfo);
    for (u32 i = 0; i < mNumScnObj; i++) {
        mpScnObjArray[i]->G3dProc(G3DPROC_CALC_MAT, param, pInfo);
    }
    CheckCallback_CALC_MAT(CALLBACK_TIMING_C, param, pInfo);
}

/* 0x80081F70 (0xC0): computes the group's view matrix and passes the view pass to the children. */
void nw4r::g3d::ScnGroup::G3dProcCalcView(u32 param, const math::MTX34* pCamera)
{
    CheckCallback_CALC_VIEW(CALLBACK_TIMING_A, param, (void*)pCamera);
    CalcViewMtx(pCamera);
    CheckCallback_CALC_VIEW(CALLBACK_TIMING_B, param, (void*)pCamera);
    for (u32 i = 0; i < mNumScnObj; i++) {
        mpScnObjArray[i]->G3dProc(G3DPROC_CALC_VIEW, param, (void*)pCamera);
    }
    CheckCallback_CALC_VIEW(CALLBACK_TIMING_C, param, (void*)pCamera);
}

/* 0x80082030 (0x6C): runs the group's work unless the pass is disabled. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnGroup::G3dProc(u32 task, u32 param, void* pInfo)
{
    if (IsG3dProcDisabled(task)) {
        return;
    }
    DefG3dProcScnGroup(task, param, pInfo);
}

/* 0x8008209C (0x18C): a group's G3dProc work: its own passes, the parent links and the child list; any other task
 * goes to every child. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnGroup::DefG3dProcScnGroup(u32 task, u32 param, void* pInfo)
{
    switch (task) {
    case G3DPROC_GATHER_SCNOBJ:
        G3dProcGatherScnObj(param, static_cast<IScnObjGather*>(pInfo));
        break;
    case G3DPROC_CALC_WORLD:
        G3dProcCalcWorld(param, static_cast<const math::MTX34*>(pInfo));
        break;
    case G3DPROC_CALC_MAT:
        G3dProcCalcMat(param, pInfo);
        break;
    case G3DPROC_CALC_VIEW:
        G3dProcCalcView(param, static_cast<const math::MTX34*>(pInfo));
        break;
    case G3DPROC_DRAW_OPA:
        break;
    case G3DPROC_DRAW_XLU:
        break;
    case G3DPROC_CHILD_DETACHED:
        Remove(static_cast<ScnObj*>(pInfo));
        break;
    case G3DPROC_DETACH_PARENT:
        SetParent(NULL);
        break;
    case G3DPROC_ATTACH_PARENT:
        if (GetParent()) {
            nw4r::db::Panic("g3d_scnobj.cpp", 0x2E6, "NW4R:Failed assertion !GetParent()");
        }
        SetParent(static_cast<G3dObj*>(pInfo));
        break;
    default:
        for (u32 i = 0; i < mNumScnObj; i++) {
            mpScnObjArray[i]->G3dProc(task, param, pInfo);
        }
        break;
    }
}

/* 0x80082228 (0x1AC): inserts a detached object at `idx` and attaches it; false when full or out of range. */
bool nw4r::g3d::ScnGroup::Insert(u32 idx, ScnObj* pObj)
{
    if (idx <= mNumScnObj && mNumScnObj < mSizeScnObj && pObj != NULL && pObj->GetParent() == NULL) {
        for (u32 i = mNumScnObj; i > idx; i--) {
            mpScnObjArray[i] = mpScnObjArray[i - 1];
        }
        mpScnObjArray[idx] = pObj;
        pObj->G3dProc(G3DPROC_ATTACH_PARENT, 0, this);
        mNumScnObj++;
        return true;
    }
    return false;
}

/* 0x800823D4 (0xBC): detaches and removes the child at `idx` and returns it. */
nw4r::g3d::ScnObj* nw4r::g3d::ScnGroup::Remove(u32 idx)
{
    if (idx < mNumScnObj) {
        ScnObj* pObj = mpScnObjArray[idx];
        pObj->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        for (u32 i = idx; i < mNumScnObj - 1; i++) {
            mpScnObjArray[i] = mpScnObjArray[i + 1];
        }
        mNumScnObj = mNumScnObj - 1;
        return pObj;
    }
    return NULL;
}

/* 0x80082490 (0x94): removes the object if it is a child; false otherwise. */
bool nw4r::g3d::ScnGroup::Remove(ScnObj* pObj)
{
    ScnObj** it = std::find(mpScnObjArray, mpScnObjArray + mNumScnObj, pObj);
    if (it != mpScnObjArray + mNumScnObj) {
        return Remove(std::distance(mpScnObjArray, it)) != NULL;
    }
    return false;
}

/* 0x80082524 (0xA8): constructs the group over a caller-owned child array; the group itself is not gathered for
 * drawing. */
nw4r::g3d::ScnGroup::ScnGroup(MEMAllocator* pHeap, ScnObj** ppObjArray, u32 capacity)
    : ScnObj(pHeap), mpScnObjArray(ppObjArray), mSizeScnObj(capacity), mNumScnObj(0)
{
    if (ppObjArray == NULL) {
        nw4r::db::Panic("g3d_scnobj.cpp", 0x344, "NW4R:Pointer must not be NULL (pBuf)");
    }
    SetScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_OPA, true);
    SetScnObjFlag(SCNOBJFLAG_NOT_GATHER_DRAW_XLU, true);
}

/* 0x800825CC (0x9C): asserts the group is detached and removes every child. */
#pragma peephole off
nw4r::g3d::ScnGroup::~ScnGroup()
{
    if (GetParent()) {
        nw4r::db::Panic("g3d_scnobj.cpp", 0x34D, "NW4R:Failed assertion !GetParent()");
    }
    Clear();
}
#pragma peephole on

/* 0x80082668 (0x44): removes children until the group is empty. */
void nw4r::g3d::ScnGroup::Clear()
{
    while (!Empty()) {
        PopBack();
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* The run-time type members                                                                        */
/* ------------------------------------------------------------------------------------------------ */

/* 0x800826AC (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnObj::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, scn_typename_ScnObj));
}

/* 0x800826DC (0x38): returns the type's name. */
const char* nw4r::g3d::ScnObj::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x80082714 (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnLeaf::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&pName, scn_typename_ScnLeaf));
}

/* 0x80082744 (0x38): returns the type's name. */
const char* nw4r::g3d::ScnLeaf::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x8008277C (0x30): returns the type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnGroup::GetTypeObj() const
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scngroup(&pName, scn_typename_ScnGroup));
}

/* 0x800827AC (0x38): returns the type's name. */
const char* nw4r::g3d::ScnGroup::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}
