/*
 * g3d/g3d_scnroot.cpp - nw4r g3d `ScnRoot` scene-graph root (camera, fog, light, the draw buffers and the scnMdl
 *   list).
 * RANGE. .text 0x800827E4-0x80084630 (49 functions); extab 0x800088F4-0x80008AFC, extabindex 0x800212C4-0x800214BC,
 *   .rodata 0x8056F6D0-0x8056F6E0 (the "ScnRoot" name record), .data 0x8058F530-0x8058F750 (opens on
 *   "g3d_scnroot.cpp"), .sdata 0x80791228-0x80791238, .sdata2 0x80795E68-0x80795E70.  The right edge is where
 *   `g3d/g3d_state.cpp` opens: fn_80084630 is the first function citing "g3d_state.cpp"; the four functions
 *   0x8008452C-0x80084630 are the class's run-time type members (the vtable in this unit's `.data` names three of
 *   them).
 * NAMES. The ScnRoot members (G3dProc, the destructor, GetTypeObj, GetTypeName, IsDerivedFrom, GetTypeObjStatic,
 *   GetCamera, SetCurrentCamera, GetFog) are nw4r's; the vtable in this unit's `.data` orders the virtuals.  The
 *   field names past ScnGroup are GUESSES (mpCollection, mDrawMode, mScnRootFlags by the constructor's stores).
 *   scnroot_align4 is a GUESS; scn_root_get_current_camera is a GUESS (C linkage: `ef/eft019.cpp` and
 *   `ef/em_effect_ctrl.cpp` call it); light_setting_dtor is a GUESS; camera_ctor is a GUESS; fog_ctor is a GUESS
 *   (the handle constructors the camera and fog accessors call); camera_data_ctor is a GUESS (0x80083488, the
 *   record constructor the ScnRoot constructor runs on each camera).  scn_typename_ScnRoot is a GUESS.
 * RESIDUALS. The ScnRoot constructor: the camera loop's counter and byte offset swap r29/r30.
 *   35 functions unwritten (objdiff scores them zero; `python tools/objdiff/unitscore.py g3d/g3d_scnroot`
 *   lists them): Construct (0x800827E4), the camera/fog/light pass (0x80082CDC), the ScnObjGather members and its std::sort instantiations
 *   (0x80083598..0x8008446C).
 *   flipcheck: `.text` short of the claim; `.rodata`, `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 * SHAPES. File-scope `#pragma peephole off` and `#pragma pool_data off` (retail keeps every `clrlwi` + `cmpwi`).
 */

#include "types.h"
#include "g3d/g3d_anmchr.h"  /* TypeObj::GetTypeName and operator==, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/fn_80075DCC.h" /* type_obj_set_name_scnleaf (rule 2) */
#include "g3d/g3d_scnobj.h"  /* nw4r::g3d::ScnGroup, owned by g3d/g3d_scnobj.cpp (rule 2) */
#include "g3d/g3d_scnroot.h" /* the full ScnRoot class (after g3d_scnobj.h) */
#include "g3d/g3d_calcworld.h" /* camera_ctor (rule 2) */
#include "g3d/g3d_camera.h"    /* camera_init, MTX44_ctor (rule 2) */
#include "fn_8004CAD8/mtx.h"   /* MTX34_ctor (rule 2) */
#include "mh3_pad.h"           /* VEC3_ctor (rule 2) */
#include "g3d/g3d_obj.h"       /* nw4r::g3d::AnmScn (rule 1) */
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */

#pragma peephole off
#pragma pool_data off

using nw4r::g3d::ScnRoot;

extern "C" CameraData* camera_data_ctor(CameraData* pData);

/* 0x80083444 (0x44): the light setting's deleting destructor (frees only for a positive flag). */
/* untyped: opaque handle - the LightSetting record */
extern "C" void* light_setting_dtor(void* pSelf, s16 flag)
{
    if (pSelf && flag > 0) {
        operator delete(pSelf);
    }
    return pSelf;
}

/* 0x80082AB4 (0xC): rounds `size` up to a multiple of 4. */
extern "C" u32 scnroot_align4(u32 size)
{
    return (size + 3) & ~3;
}

/* 0x80082AC0 (0xB0): drops a detaching scene animation, otherwise runs the group's default processing. */
void ScnRoot::G3dProc(u32 task, u32 param, void* pInfo) /* untyped: caller-owned payload */
{
    if (IsG3dProcDisabled(task)) {
        return;
    }
    if (task == G3DPROC_CHILD_DETACHED && mpAnmScn == pInfo) {
        mpAnmScn->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
        mpAnmScn = NULL;
        return;
    }
    DefG3dProcScnGroup(task, param, pInfo);
}

/* 0x80083328 (0x11C): builds the root over its child array, draw collection and light buffers, and initialises
 * every camera and fog record. */
/* untyped: opaque handle - the draw collection, the light-object buffers */
ScnRoot::ScnRoot(MEMAllocator* pHeap, void* pCollection, ScnObj** ppChildren, u32 maxChildren, u32 numLightObj,
                 u32 numAmbLightObj, void* pLightObjBuf, void* pLightAnmBuf, void* pAmbLightObjBuf)
    : ScnGroup(pHeap, ppChildren, maxChildren), mpCollection(pCollection), mDrawMode(2), mScnRootFlags(0),
      mCurrentCameraID(0)
{
    CameraData* pCamera = mCamera;
    do {
        camera_data_ctor(pCamera);
        pCamera++;
    } while (pCamera < &mCamera[32]);
    light_setting_ctor(mLightSetting, (s32)pLightObjBuf, (s32)pLightAnmBuf, numLightObj, (s32)pAmbLightObjBuf,
                       numAmbLightObj);
    mpAnmScn = NULL;
    u32 i;
    for (i = 0; i < 32; i++) {
        ResHandle camera;
        camera_ctor(&camera, (u32)(mCamera + i));
        camera_init((nw4r::g3d::Camera*)&camera);
    }
    for (i = 0; i < 32; i++) {
        s32 fog;
        fog_ctor((s32)&fog, mFog[i]);
        fog_init((s32)&fog);
    }
}

/* 0x80083488 (0x68): constructs a camera record's matrix and vector members. */
extern "C" CameraData* camera_data_ctor(CameraData* pData)
{
    MTX34_ctor((nw4r::math::MTX34*)pData->mViewMtx);
    MTX44_ctor((nw4r::math::MTX44*)pData->mProjMtx);
    VEC3_ctor((nw4r::math::VEC3*)&pData->mPosX);
    VEC3_ctor((nw4r::math::VEC3*)&pData->mTargetX);
    VEC3_ctor((nw4r::math::VEC3*)&pData->mUpX);
    VEC3_ctor((nw4r::math::VEC3*)&pData->mUnk98);
    VEC2_ctor(&pData->mViewportX);
    VEC2_ctor(&pData->mViewportW);
    return pData;
}

/* 0x80082B70 (0x5C): camera `index` (a null camera outside [0, 32)). */
int ScnRoot::GetCamera(int index)
{
    if (index >= 0 && 32 > index) {
        ResHandle camera;
        return (int)camera_ctor(&camera, (u32)&mCamera[index])->mpData;
    }
    ResHandle none;
    return (int)camera_ctor(&none, 0)->mpData;
}

/* 0x80082BCC (0x3C): the root's current camera. */
extern "C" s32 scn_root_get_current_camera(s32 root)
{
    ScnRoot* pRoot = (ScnRoot*)root;
    ResHandle camera;
    return (s32)camera_ctor(&camera, (u32)&pRoot->mCamera[pRoot->mCurrentCameraID])->mpData;
}

/* 0x80082C08 (0x78): makes camera `camera` current. */
void ScnRoot::SetCurrentCamera(int camera)
{
    bool valid = false;
    if (camera >= 0 && 32 > camera) {
        valid = true;
    }
    if (!valid) {
        nw4r::db::Panic("g3d_scnroot.cpp", 0x9C, "NW4R:Failed assertion 0 <= camID && camID < NUM_CAMERA");
    }
    mCurrentCameraID = camera;
}

/* 0x80082C80 (0x5C): fog `index` (a null fog outside [0, 32)). */
int ScnRoot::GetFog(int index)
{
    if (index >= 0 && 32 > index) {
        s32 fog;
        return *(s32*)fog_ctor((s32)&fog, mFog[index]);
    }
    s32 none;
    return *(s32*)fog_ctor((s32)&none, NULL);
}

/* 0x800834F4 (0xA4): detaches the scene animation and destroys the light setting. */
ScnRoot::~ScnRoot()
{
    if (mpAnmScn) {
        mpAnmScn->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
    }
    light_setting_dtor(mLightSetting, -1);
}

/* 0x8008452C (0x30): the ScnRoot type object. */
const nw4r::g3d::G3dObj::TypeObj ScnRoot::GetTypeObj() const
{
    const u8* local;

    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&local, (const u8*)scn_typename_ScnRoot));
}

/* 0x8008455C (0x38): returns the type's name. */
const char* ScnRoot::GetTypeName() const
{
    return GetTypeObj().GetTypeName();
}

/* 0x80084594 (0x6C): whether the object is a ScnRoot or derives from `type`. */
bool ScnRoot::IsDerivedFrom(TypeObj type) const
{
    if (type == GetTypeObjStatic()) {
        return true;
    }
    return ScnGroup::IsDerivedFrom(type);
}

/* 0x80084600 (0x30): the ScnRoot type object. */
const nw4r::g3d::G3dObj::TypeObj ScnRoot::GetTypeObjStatic()
{
    const u8* local;

    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&local, (const u8*)scn_typename_ScnRoot));
}


