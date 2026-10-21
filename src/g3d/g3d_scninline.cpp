/*
 * g3d/g3d_scninline.cpp - the kept copies of nw4r g3d's scene-object inline members: ScnGroup::Size and its
 *   std::find/std::distance instances, ScnObj's CheckCallback_* hooks, flag and matrix accessors, the ScnObj/ScnGroup/
 *   ScnLeaf run-time type members, the ScnLeaf constructor and destructor, the scene gather interface's constructor
 *   and destructors, and the ResMdl/ResShp handle helpers.
 * RANGE. .text 0x8007B424-0x8007BB94 (42 functions); extab 0x800084C0-0x80008540, extabindex 0x80020D48-0x80020E08, .data
 *   0x8058ED38-0x8058EDA0 (IScnObjGather's vtable, then the ResMdl constructor's inline tail
 *   "NW4R:Failed assertion !((u32)p & 0x1f)" / "g3d_resmdl_ac.h"), .sdata2 0x80795E58-0x80795E60 (the ScnLeaf
 *   constructor's 1.0f: a second pooled 1.0 after `g3d/g3d_light.cpp`'s 0x80795E34, so a different TU).  The
 *   `.data` run opens on a vtable whose first code slot (0x8007B7F0) is above `g3d/g3d_obj.cpp`'s (a new TU) and closes
 *   on an inline tail with no `__FILE__` string of its own; `g3d/g3d_scnmdl.cpp` opens on "g3d_scnmdl.cpp" at
 *   0x8058EDA0 and on its own ScnMdl members at 0x8007BB94.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106).
 * NAMES. The file name is a GUESS: no function here cites a `__FILE__` string, and every one is an inline member
 *   other units call (CheckCallback_* from g3d_scnobj/g3d_scnmdl/g3d_scnmdlsmpl, the ScnLeaf constructor and
 *   destructor from ScnMdlSimple's), so the TU's own non-inline functions are not in the DOL; nw4r's
 *   g3d_scnproc.cpp (a ScnLeaf whose G3dProc runs every CheckCallback_* hook) is the candidate.
 *   type_obj_set_name_scnleaf and type_obj_set_name_scngroup are GUESSES (the type-name store copies the run-time type
 *   members call).  res_mdl_ctor_referencer is a GUESS (the dead-stripped caller that places the ResMdl constructor).
 *   IScnObjCallback's empty ExecCallback_CALC_VIEW/CALC_MAT/CALC_WORLD/DRAW_XLU/DRAW_OPA are the slots of
 *   `lobby/lb_npc.cpp`'s callback vtables 0x805B8D80/0x805B8DA0.
 * RESIDUALS. fn_8007B864, fn_8007B878, fn_8007B8DC, fn_8007BA08 and fn_8007BA38 keep the map stems: the ResMdl handle
 *   constructor (fn_8007B878, asserting the `g3d_resmdl_ac.h` alignment) and two handle copies other units call by
 *   name.
 * SHAPES. Definitions follow retail's address order.  The ResMdl constructor is `inline` and placed by a 4-byte
 *   referencer the link dead-strips, so its two strings are the weak inline tail after the vtable (retail's `.data`).
 *   setVec3 is defined `inline` ahead of the ScnLeaf constructor (called out of line under `dont_inline`): the visible
 *   body cannot throw, so the constructor has no unwind range (retail's extab record).
 */

#include "types.h"


#include "sys_mem.h" /* operator delete (rule 9: call through the owner) */
#include "nw4r/g3d/scnmdl.h" /* nw4r::g3d::ScnMdl::CopiedMatAccess - the owner of the two mangled members (rule 1/9) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "g3d/g3d_xsi.h" /* g3d_calc_tex_mtx_xsi (rule 2) */
#include "g3d/g3d_obj.h" /* nw4r::g3d::G3dObj (rule 2) */
#include "MEM/MEMAllocFromAllocator.h" /* MEMFreeToAllocator (rule 2) */
#include "g3d/g3d_anmchr.h" /* type_obj_set_name (rule 2) */
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_resnode.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_state.h"     /* the material loaders (rule 2) */
#include "nw4r/fn_805012C4.h"
#include "g3d/g3d_scnobj.h"   /* nw4r::g3d::ScnObj / ScnLeaf / ScnGroup / IScnObjGather (rule 1) */
#include "g3d/g3d_scnroot.h"  /* nw4r::g3d::ScnObjGather (rule 1) */
#include "g3d/g3d_scnmdlsmpl.h" /* nw4r::g3d::ScnMdlSimple (rule 1) */
#include "MSL/algorithm.h"     /* std::find / std::distance */
#include "unsplit/g3d.h"      /* the scene objects' type-name records no unit owns (rule 2) */
#include "nw4r/db_assert.h"   /* nw4r::db::Panic (rule 2) */

#pragma peephole off

extern "C" {
/* internal */ const u8** type_obj_set_name_scngroup(const u8** out, const u8* v);
/* internal */ struct ResHandle* res_shp_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc);
/* internal */ void fn_8007B864(struct ResHandle* pDst, const struct ResHandle* pSrc);
/* internal */ u32 fn_8007B878(s32 pDst, s32 offset);
/* internal */ void fn_8007B8DC(struct ResHandle* pSelf, s32 offset);
/* internal */ struct ResHandle* fn_8007BA08(struct ResHandle* pDst, const struct ResHandle* pSrc);
/* internal */ void fn_8007BA38(struct ResHandle* pDst, const struct ResHandle* pSrc);
/* internal */ const u8** type_obj_set_name_scnleaf(const u8** out, const u8* v);
} /* extern "C" */

/* 0x8007B424 (0x8): the child count. */
u32 nw4r::g3d::ScnGroup::Size() const
{
    return mNumScnObj;
}

/* 0x8007B42C/0x8007B450/0x8007B47C: the std::find and std::distance instances ScnGroup::Remove(ScnObj*) calls. */
template nw4r::g3d::ScnObj** std::find(nw4r::g3d::ScnObj** first, nw4r::g3d::ScnObj** last,
                                       nw4r::g3d::ScnObj* const& value);
template long std::distance(nw4r::g3d::ScnObj** first, nw4r::g3d::ScnObj** last);
template long std::__distance(nw4r::g3d::ScnObj** first, nw4r::g3d::ScnObj** last, std::random_access_iterator_tag);

/* 0x8007B48C (0x58): runs the callback's CALC_VIEW hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_CALC_VIEW(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_CALC_VIEW) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_CALC_VIEW(timing, this, param, pInfo);
    }
}

/* 0x8007B4E4 (0x4): the callback's default CALC_VIEW hook does nothing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::IScnObjCallback::ExecCallback_CALC_VIEW(u32 timing, ScnObj* pObj, u32 param, void* pInfo)
{
}

/* 0x8007B4E8 (0x58): runs the callback's CALC_MAT hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_CALC_MAT(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_CALC_MAT) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_CALC_MAT(timing, this, param, pInfo);
    }
}

/* 0x8007B540 (0x4): the callback's default CALC_MAT hook does nothing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::IScnObjCallback::ExecCallback_CALC_MAT(u32 timing, ScnObj* pObj, u32 param, void* pInfo)
{
}

/* 0x8007B544 (0x20): the matrix of `type`, or NULL for an unknown type. */
nw4r::math::MTX34* nw4r::g3d::ScnObj::GetMtxPtr(ScnObjMtxType type)
{
    if ((u32)type < MTX_TYPE_MAX) {
        return &mMtxArray[type];
    }
    return NULL;
}

/* 0x8007B564 (0x58): runs the callback's CALC_WORLD hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_CALC_WORLD(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_CALC_WORLD) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_CALC_WORLD(timing, this, param, pInfo);
    }
}

/* 0x8007B5BC (0x4): the callback's default CALC_WORLD hook does nothing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::IScnObjCallback::ExecCallback_CALC_WORLD(u32 timing, ScnObj* pObj, u32 param, void* pInfo)
{
}

/* 0x8007B5C0 (0x34): whether the object's flags disable G3dProc pass `task`. */
bool nw4r::g3d::ScnObj::IsG3dProcDisabled(u32 task) const
{
    if (task < 9 && ((1 << (task - 1)) & mScnObjFlags)) {
        return true;
    }
    return false;
}

/* 0x8007B5F4 (0x6C): whether the object is a ScnGroup or derives from `type`. */
bool nw4r::g3d::ScnGroup::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnObj::IsDerivedFrom(type);
}

/* 0x8007B660 (0x6C): whether the object is a ScnObj or derives from `type`. */
bool nw4r::g3d::ScnObj::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return G3dObj::IsDerivedFrom(type);
}

/* 0x8007B6CC (0x30): returns the ScnObj type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnObj::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name(&pName, scn_typename_ScnObj));
}

/* 0x8007B6FC (0x30): returns the ScnGroup type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnGroup::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scngroup(&pName, scn_typename_ScnGroup));
}

extern "C" {
/* 0x8007B72C (0x8): stores `v` through `out` and returns `out`. */
const u8** type_obj_set_name_scngroup(const u8** out, const u8* v)
{
    *out = v;
    return out;
}
}   /* extern "C" */

/* 0x8007B734 (0x30): returns the ScnMdlSimple type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnMdlSimple::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(
        type_obj_set_name_anmchr(&pName, reinterpret_cast<const u8*>(&scn_typename_ScnMdlSimple)));
}

/* 0x8007B764 (0x30): returns the ScnMdl type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnMdl::GetTypeObjStatic()
{
    void* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name((const u8**)&pName, scn_typename_ScnMdl));
}

/* 0x8007B794: destroys the draw collector (its lists are the caller's buffers). */
nw4r::g3d::ScnObjGather::~ScnObjGather()
{
}

/* 0x8007B7F0: destroys the collector interface. */
nw4r::g3d::IScnObjGather::~IScnObjGather()
{
}

extern "C" {

/* 0x8007B834 (0x30): copy-constructs a handle through its base-handle copy and returns the destination. */
struct ResHandle* res_shp_copy_ctor(struct ResHandle* pDst, const struct ResHandle* pSrc) {
    fn_8007B864(pDst, pSrc);
    return pDst;
}

/* 0x8007B864 (0xC): copies the base handle's data pointer. */
void fn_8007B864(struct ResHandle* pDst, const struct ResHandle* pSrc) {
    pDst->mpData = pSrc->mpData;
}

}   /* extern "C": the placement `operator new` below has C++ linkage (`__nw__FUlPv`) */

/* The placement `operator new`: the caller's address back, unchanged. */
/* untyped: opaque handle passed through - the placement address the caller hands in */
void* operator new(unsigned long size, void* place) {
    return place;
}

extern "C" {

/* 0x8007B878 (0x64): the ResMdl handle constructor: stores the block address, asserting its 32-byte alignment. */
inline u32 fn_8007B878(s32 pDst, s32 offset) {
    fn_8007B8DC((struct ResHandle*)pDst, offset);
    if ((offset & 0x1F) != 0) {
        nw4r::db::Panic("g3d_resmdl_ac.h", 0x78, "NW4R:Failed assertion !((u32)p & 0x1f)");
    }
    return pDst;
}

#pragma dont_inline on
/* Calls the ResMdl handle constructor (the link dead-strips it; it places the constructor's kept copy). */
u32 res_mdl_ctor_referencer(s32 pDst, s32 offset) {
    return fn_8007B878(pDst, offset);
}
#pragma dont_inline reset

/* 0x8007B8DC (0x8): stores the block address. */
void fn_8007B8DC(struct ResHandle* pSelf, s32 offset) {
    pSelf->mpData = (void*)offset;
}

}   /* extern "C" */

/* 0x8007B8E4 (0x58): runs the callback's DRAW_XLU hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_DRAW_XLU(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_DRAW_XLU) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_DRAW_XLU(timing, this, param, pInfo);
    }
}

/* 0x8007B93C (0x4): the callback's default DRAW_XLU hook does nothing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::IScnObjCallback::ExecCallback_DRAW_XLU(u32 timing, ScnObj* pObj, u32 param, void* pInfo)
{
}

/* 0x8007B940 (0x58): runs the callback's DRAW_OPA hook at `timing` when it is enabled for the pass and the timing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::ScnObj::CheckCallback_DRAW_OPA(Timing timing, u32 param, void* pInfo)
{
    if (mpFnCallback != NULL && (mCallbackExecOpMask & EXECOP_DRAW_OPA) && (mCallbackTiming & timing)) {
        mpFnCallback->ExecCallback_DRAW_OPA(timing, this, param, pInfo);
    }
}

/* 0x8007B998 (0x4): the callback's default DRAW_OPA hook does nothing. */
/* untyped: caller-owned payload - the pass's info block */
void nw4r::g3d::IScnObjCallback::ExecCallback_DRAW_OPA(u32 timing, ScnObj* pObj, u32 param, void* pInfo)
{
}

/* 0x8007B99C: constructs the collector interface. */
nw4r::g3d::IScnObjGather::IScnObjGather()
{
}

/* 0x8007B9AC: empties both draw lists. */
void nw4r::g3d::ScnObjGather::Clear()
{
    mNumScnObjXlu = 0;
    mNumScnObjOpa = 0;
}

/* 0x8007B9BC (0x18): whether any of `flag`'s bits is set. */
u32 nw4r::g3d::ScnObj::TestScnObjFlag(ScnObjFlag flag) const
{
    return (mScnObjFlags & flag) != 0;
}

/* 0x8007B9D4 (0x2C): sets or clears `flag`'s bits. */
void nw4r::g3d::ScnObj::SetScnObjFlag(ScnObjFlag flag, u32 on)
{
    if (on) {
        mScnObjFlags |= flag;
    } else {
        mScnObjFlags &= ~flag;
    }
}

/* 0x8007BA00: whether the user sorted the lists (the base never has). */
bool nw4r::g3d::IScnObjGather::IsUserSorted() const
{
    return false;
}

extern "C" {

/* 0x8007BA08 (0x30): copy-constructs a handle through its base-handle copy and returns the destination. */
struct ResHandle* fn_8007BA08(struct ResHandle* pDst, const struct ResHandle* pSrc) {
    fn_8007BA38(pDst, pSrc);
    return pDst;
}

/* 0x8007BA38 (0xC): copies the base handle's data pointer. */
void fn_8007BA38(struct ResHandle* pDst, const struct ResHandle* pSrc) {
    pDst->mpData = pSrc->mpData;
}

}   /* extern "C" */

/* 0x8007BA44 (0x5C): destroys the leaf. */
nw4r::g3d::ScnLeaf::~ScnLeaf()
{
}

/* The vector's three-component constructor (its kept copy is `mh3_pad.cpp`'s 0x80041E8C), visible so the compiler
 * knows it does not throw. */
extern "C" inline nw4r::math::VEC3* setVec3(nw4r::math::VEC3* out, float x, float y, float z)
{
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

#pragma dont_inline on
/* 0x8007BAA0 (0x50): constructs the leaf with a unit scale. */
nw4r::g3d::ScnLeaf::ScnLeaf(MEMAllocator* pHeap) : ScnObj(pHeap)
{
    setVec3(&mScale, 1.0f, 1.0f, 1.0f);
}
#pragma dont_inline reset

/* 0x8007BAF0 (0x6C): whether the object is a ScnLeaf or derives from `type`. */
bool nw4r::g3d::ScnLeaf::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnObj::IsDerivedFrom(type);
}

/* 0x8007BB5C (0x30): returns the ScnLeaf type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::ScnLeaf::GetTypeObjStatic()
{
    const u8* pName;
    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&pName, scn_typename_ScnLeaf));
}

extern "C" {
/* 0x8007BB8C (0x8): stores `v` through `out` and returns `out`. */
const u8** type_obj_set_name_scnleaf(const u8** out, const u8* v)
{
    *out = v;
    return out;
}
}   /* extern "C" */
