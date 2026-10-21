/*
 * g3d/g3d_scnroot.cpp - nw4r g3d `ScnRoot` scene-graph root (camera, fog, light, the draw buffers and the scnMdl
 *   list) and its draw collector `ScnObjGather`.
 * RANGE. .text 0x800827E4-0x80084630 (49 functions); extab 0x800088F4-0x80008AFC, extabindex 0x800212C4-0x800214BC,
 *   .rodata 0x8056F6D0-0x8056F6E0 (the "ScnRoot" name record), .data 0x8058F530-0x8058F750 (opens on
 *   "g3d_scnroot.cpp"), .sdata 0x80791228-0x80791238, .sdata2 0x80795E68-0x80795E70.  The right edge is where
 *   `g3d/g3d_state.cpp` opens: fn_80084630 is the first function citing "g3d_state.cpp"; the four functions
 *   0x8008452C-0x80084630 are the class's run-time type members (the vtable in this unit's `.data` names three of
 *   them).
 * NAMES. math_f32_to_u16 is a GUESS; os_f32_to_u16_store is a GUESS; os_f32_to_u16_cast is a GUESS;
 *   vec3_ctor_return is a GUESS; scn_root_calc_world is a GUESS; scn_root_calc_material is a GUESS;
 *   scn_root_apply_anm_scn is a GUESS (nw4r's CalcAnmScn: the three ref-number slots, then the light setting, the
 *   fogs and the cameras the scene animation drives).
 *   The ScnRoot members (Construct, G3dProc, SetGlbSettings, the destructor, GetTypeObj, GetTypeName, IsDerivedFrom,
 *   GetTypeObjStatic, GetCamera, SetCurrentCamera, GetFog) and the ScnObjGather members (Add, ZSort, Sort, DrawOpa,
 *   DrawXlu) are nw4r's; the vtables in this unit's `.data` order the virtuals.  IsUserSorted is a GUESS (the ninth
 *   slot; both bodies return false).  The field names past ScnGroup are GUESSES (mpCollection, mDrawMode,
 *   mScnRootFlags by the constructor's stores).
 *   scnroot_align4 is a GUESS; scn_root_get_current_camera is a GUESS (C linkage: `ef/eft019.cpp` and
 *   `ef/em_effect_ctrl.cpp` call it); the CameraData constructor (0x80083488) and LightSetting's destructor
 *   (0x80083444) are the kept copies of the two members' inline constructor/destructor.  scn_typename_ScnRoot is a
 *   GUESS.
 *   fog_is_range_adj_enable, fog_ref, fog_set_near_far and camera_get_projection_type are GUESSES (the Fog/Camera
 *   inline copies SetGlbSettings calls: the "g3d_fog.h" asserts, the +0x18/+0x0C/+0x10/+0xA8 fields).
 *   scnroot_sort, scnroot_insertion_sort, scnroot_min_element, scnroot_median3, scnroot_iter_swap, scnroot_move are
 *   GUESSES (the MSL introsort instances over the object lists: by-value and by-reference comparator, the selection
 *   pass, the median-of-three pivot, the swap and its identity cast).
 *   scnroot_less_z_opa, scnroot_less_z_xlu, scnroot_less_sort_opa, scnroot_less_sort_xlu are GUESSES (the four
 *   comparators ZSort and Sort pass: draw priority first, then view depth or the object's sort value).
 *   scnroot_num_fog and scnroot_num_camera are GUESSES (the `.sdata` words 32 the std::min calls take by address).
 * RESIDUALS. Construct: register colouring only - retail gives the size offsets r16..r21 and the parameters r22..r27, ours
 *   the reverse (tried: pRoot and listSize declared later/earlier).
 *   scnroot_sort (both instances): i/j/pivot rotate r26..r28 against retail's (tried: declaration orders, the
 *   second-pass `j` reloaded, a separate pivot local; `offsetB` as a local fixes the 3/4 offset's scheduling).
 *   flipcheck: `.text`/extab only (the rows above; the scnroot_sort/iter_swap pair is emitted in the other
 *   order); `.rodata`, `.data`, `.sdata` and `.sdata2` byte-identical.
 * SHAPES. File-scope `#pragma peephole off`, `#pragma pool_data off` (retail keeps every `clrlwi` + `cmpwi`) and
 *   `#pragma fp_contract off` (SetGlbSettings keeps `fmuls` + `fadds`).  The Fog/Camera inline copies are `inline`
 *   definitions ahead of SetGlbSettings under `#pragma dont_inline on`: the weak copies follow their referencer in
 *   retail's order, and their `@STRING@` tail (range-adjust, near/far, ref) needs near/far defined before ref.
 *   GetPriorityDraw* return `int` (the comparators' signed compare); Add post-increments the list counts.
 *   The camera records and the light setting are members with constructors (the compiler builds the camera array
 *   first, then the light setting, and records the light setting's destructor in the constructor's and the
 *   destructor's unwind actions); the constructor and destructor compile under `dont_inline`.  The math types'
 *   empty constructors, VEC2_ctor and G3dObj's operator delete are visible inline bodies so their calls get no
 *   unwind range (their weak copies other than VEC2_ctor are dropped by the link).  `#pragma peephole off` comes
 *   before the includes so the LightSetting destructor's kept copy keeps `extsh` + `cmpwi`.
 */

#pragma peephole off

#include "types.h"
#include "OS/OSFastCast.h" /* OS_FASTCAST_U16 (rule 2) */
#include "g3d/g3d_anmchr.h"  /* TypeObj::GetTypeName and operator==, owned by g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_anmscn.h" /* placement delete */
#include "g3d/light_obj_is_enable.h" /* fog_is_valid / fog_ptr / g3d_min_u32, owned by g3d/g3d_anmscn.cpp (rule 2) */
#include "g3d/g3d_fog.h" /* fog_ctor / fog_init, nw4r::g3d::Fog (rule 2) */
#include "g3d/g3d_light.h" /* light_setting_ctor (rule 2) */
#include "g3d/g3d_scninline.h" /* the scene inline copies, placement new (rule 2) */
#include "g3d/g3d_scnobj.h"  /* nw4r::g3d::ScnGroup, owned by g3d/g3d_scnobj.cpp (rule 2) */
#include "g3d/g3d_scnroot.h" /* the full ScnRoot class (after g3d_scnobj.h) */
#include "g3d/g3d_scnmdlsmpl.h" /* AABB_ctor (rule 2) */
#include "g3d/g3d_calcworld.h" /* camera_ctor (rule 2) */
#include "g3d/g3d_camera.h"    /* camera_init, MTX44_ctor, the camera accessors (rule 2) */
#include "g3d/g3d_camera_types.h" /* nw4r::g3d::Camera, AnmScn::GetCamera's by-value handle (rule 1) */
#include "g3d/g3d_state.h"     /* the light, camera and fog setters (rule 2) */
#include "fn_8004CAD8/mtx.h"   /* MTX34_ctor (rule 2) */
#include "mh3_pad.h"           /* VEC3_ctor, word_copy_return_dst (rule 2) */
#include "g3d/g3d_obj.h"       /* nw4r::g3d::AnmScn (rule 1) */
#include "nw4r/g3d/res_common.h" /* ResHandle (rule 1) */
#include "nw4r/math_geometry.h" /* nw4r::math::Frustum (rule 2) */
#include "nw4r/db_assert.h" /* nw4r::db::Panic (rule 2) */

#pragma pool_data off
#pragma fp_contract off

using nw4r::g3d::ScnRoot;
using nw4r::g3d::ScnObj;
using nw4r::g3d::ScnObjGather;
using nw4r::g3d::IScnObjGather;


/* nw4r's pointer check: a valid pointer lies in MEM1, MEM2 or the locked cache. */
#define SCNROOT_POINTER_ASSERT(ptr, line, msg)                                                  \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic("g3d_scnroot.cpp", line, msg, (ptr));                                \
    }

extern "C" {
u32 scnroot_align4(u32 size);
s32 fog_is_range_adj_enable(const ResHandle* pFog);
void fog_set_near_far(ResHandle* pFog, f32 nearz, f32 farz);
s32 camera_get_projection_type(nw4r::g3d::Camera* pCamera);
u16 os_f32_to_u16_cast(register f32 arg);
void os_f32_to_u16_store(f32* in, u16* out);
bool scnroot_less_z_opa(ScnObj* pLhs, ScnObj* pRhs);
bool scnroot_less_z_xlu(ScnObj* pLhs, ScnObj* pRhs);
bool scnroot_less_sort_opa(ScnObj* pLhs, ScnObj* pRhs);
bool scnroot_less_sort_xlu(ScnObj* pLhs, ScnObj* pRhs);
}

/* 0x8056F6D0: the "ScnRoot" type-name record (a length word, then the name) the run-time type members read. */
extern "C" const nw4r::g3d::TypeNameData<sizeof("ScnRoot")> scn_typename_ScnRoot = {sizeof("ScnRoot"), "ScnRoot"};

/* The fog and camera counts CalcAnmScn clamps the animation's reference numbers to (`.sdata`). */
static u32 scnroot_num_fog = 32;
static u32 scnroot_num_camera = 32;

/* 0x800827E4 (0x2D0): sizes a root with its gather lists, child array and light buffers, reports the size through
 * `pSize`, and builds root and collector in one block from `pHeap`. */
ScnRoot* ScnRoot::Construct(MEMAllocator* pHeap, u32* pSize, u32 maxChildren, u32 maxScnObj, u32 numLightObj,
                            u32 numLightSet)
{
    ScnRoot* pRoot = NULL;
    u32 gatherOffset = scnroot_align4(sizeof(ScnRoot));
    u32 childrenOffset = scnroot_align4(gatherOffset + sizeof(ScnObjGather));
    u32 opaOffset = scnroot_align4(childrenOffset + maxChildren * sizeof(ScnObj*));
    u32 listSize = maxScnObj * sizeof(ScnObj*);
    u32 lightObjOffset = scnroot_align4(opaOffset + listSize * 2);
    u32 lightAnmOffset = scnroot_align4(lightObjOffset + numLightObj * 0x44);
    u32 ambLightOffset = scnroot_align4(lightAnmOffset + numLightObj * 4);
    u32 size = scnroot_align4(ambLightOffset + numLightSet * 12);
    if (pSize != NULL) {
        SCNROOT_POINTER_ASSERT(pSize, 0x4A, "NW4R:Pointer Error\npSize(=%p) is not valid pointer.");
        *pSize = size;
    }
    if (pHeap != NULL) {
        u8* pBuf = (u8*)Alloc(pHeap, size);
        if ((u32)pBuf & 3) {
            nw4r::db::Panic("g3d_scnroot.cpp", 0x55, "NW4R:Failed assertion ((u32)buf & 0x3) == 0");
        }
        if (pBuf == NULL) {
            return NULL;
        }
        ScnObj** ppOpa = (ScnObj**)(pBuf + opaOffset);
        ScnObjGather* pGather =
            new (pBuf + gatherOffset) ScnObjGather(ppOpa, (ScnObj**)((u8*)ppOpa + listSize), maxScnObj);
        u8* pLightObjBuf = pBuf + lightObjOffset;
        u8* pLightAnmBuf = pBuf + lightAnmOffset;
        u8* pAmbLightBuf = pBuf + ambLightOffset;
        pRoot = new (pBuf) ScnRoot(pHeap, pGather, (ScnObj**)(pBuf + childrenOffset), maxChildren, numLightObj,
                                   numLightSet, pLightObjBuf, pLightAnmBuf, pAmbLightBuf);
    }
    return pRoot;
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

#pragma dont_inline on
/* 0x80082F18 (0x30): the float as an unsigned 16-bit integer. */
extern "C" inline u16 math_f32_to_u16(f32 value)
{
    u16 result;
    os_f32_to_u16_store(&value, &result);
    return result;
}

/* 0x80082F48 (0x34): stores the converted float through `out`. */
extern "C" inline void os_f32_to_u16_store(f32* in, u16* out)
{
    *out = os_f32_to_u16_cast(*in);
}

/* 0x80082F7C (0x18): the float as an unsigned 16-bit integer through the quantised store. */
extern "C" inline u16 os_f32_to_u16_cast(register f32 arg)
{
    f32 a;
    register f32* ptr = &a;
    register u16 r;
    asm {
        psq_st arg, 0(ptr), 1, OS_FASTCAST_U16
        lhz r, 0(ptr)
    }
    return r;
}

/* 0x80082F94 (0x88): whether the fog's range adjustment is on (asserting a valid handle). */
extern "C" inline s32 fog_is_range_adj_enable(const ResHandle* pFog)
{
    if (!fog_is_valid((const nw4r::g3d::Fog*)pFog)) {
        nw4r::db::Panic("g3d_fog.h", 0x7F, "NW4R:Failed assertion IsValid()");
    }
    bool enable = false;
    if (fog_is_valid((const nw4r::g3d::Fog*)pFog) && ((const nw4r::g3d::Fog*)pFog)->ptr()->adjEnable == 1) {
        enable = true;
    }
    return enable;
}

/* 0x80083070 (0x88): sets the fog's near and far depths (asserting a valid handle). */
extern "C" inline void fog_set_near_far(ResHandle* pFog, f32 nearz, f32 farz)
{
    if (!fog_is_valid((const nw4r::g3d::Fog*)pFog)) {
        nw4r::db::Panic("g3d_fog.h", 0x57, "NW4R:Failed assertion IsValid()");
    }
    if (fog_is_valid((const nw4r::g3d::Fog*)pFog)) {
        FogData* pData = (FogData*)fog_ptr((const nw4r::g3d::Fog*)pFog);
        pData->nearz = nearz;
        pData->farz = farz;
    }
}

/* 0x8008301C (0x54): the fog record, asserting a non-null handle. */
extern "C" inline FogData* fog_ref(ResHandle* pFog)
{
    if (pFog->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 0x8C, "NW4R:Pointer must not be NULL (mpData)");
    }
    return (FogData*)pFog->mpData;
}

/* 0x800830F8 (0x24): the camera's projection type. */
extern "C" inline s32 camera_get_projection_type(nw4r::g3d::Camera* pCamera)
{
    return camera_ref_const(pCamera)->mProjType;
}

/* 0x80082CDC (0x23C): loads the current camera into GX, records every camera, every fog (adjusted to the camera's
 * depth range and projection) and the light setting in the draw state. */
void ScnRoot::SetGlbSettings()
{
    nw4r::g3d::Camera camera;
    ResHandle fog;
    f32 w;
    f32 x;
    s32 current;
    ResHandle handle;
    current = scn_root_get_current_camera((s32)this);
    word_copy_return_dst(&camera, &current);
    camera_gx_set_viewport(&camera);
    camera_gx_set_projection(&camera);
    camera_gx_set_scissor(&camera);
    camera_gx_set_scissor_box_offset(&camera);
    int i;
    for (i = 0; i < 32; i++) {
        g3d_state_set_camera(*(nw4r::g3d::Camera*)camera_ctor(&handle, (u32)&mCamera[i]), i,
                             i == mCurrentCameraID);
    }
    bool perspective = camera_get_projection_type(&camera) != 1;
    u32 orthoBit = perspective ? 0 : 8;
    f32 nearz = camera_ref(&camera)->mProjC;
    f32 farz = camera_ref(&camera)->mProjD;
    u16 width = 0;
    u16 center = 0;
    nw4r::math::MTX44 projMtx;
    MTX44_ctor(&projMtx);
    bool calculated = false;
    for (i = 0; i < 32; i++) {
        fog_ctor((s32)&fog, mFog[i]);
        fog_set_near_far(&fog, nearz, farz);
        if (fog_ref(&fog)->type != 0) {
            fog_ref(&fog)->type = orthoBit | (fog_ref(&fog)->type & ~8);
        }
        if (fog_is_range_adj_enable(&fog)) {
            if (!calculated) {
                camera_get_projection_mtx(&camera, &projMtx);
                camera_get_viewport(&camera, &x, NULL, &w, NULL, NULL, NULL);
                width = math_f32_to_u16(w);
                center = math_f32_to_u16(x + 0.5f * w);
                calculated = true;
            }
            ((nw4r::g3d::Fog*)&fog)->SetFogRangeAdjParam(width, center, projMtx);
        }
        g3d_state_set_fog(*(G3dFog*)&fog, i);
    }
    g3d_state_set_light_setting(&mLightSetting);
}

#pragma dont_inline reset

/* 0x8008311C (0x174): copies the bound scene animation's light setting, fogs and cameras into the root. */
extern "C" void scn_root_apply_anm_scn(ScnRoot* pRoot)
{
    if (pRoot->mpAnmScn == NULL) {
        return;
    }
    u32 refLightSet = pRoot->mpAnmScn->GetLightSetMaxRefNumber();
    u32 refFog = pRoot->mpAnmScn->GetFogMaxRefNumber();
    u32 refCamera = pRoot->mpAnmScn->GetCameraMaxRefNumber();
    if (refLightSet != 0) {
        pRoot->mpAnmScn->GetLightSetting(&pRoot->mLightSetting);
    }
    if (refFog != 0) {
        u32 num = *g3d_min_u32(&scnroot_num_fog, &refFog);
        for (u32 i = 0; i < num; i++) {
            ResHandle fog;
            fog_ctor((s32)&fog, pRoot->mFog[i]);
            pRoot->mpAnmScn->GetFog(*(nw4r::g3d::Fog*)&fog, i);
        }
    }
    if (refCamera != 0) {
        u32 num = *g3d_min_u32(&scnroot_num_camera, &refCamera);
        for (u32 i = 0; i < num; i++) {
            ResHandle camera;
            camera_ctor(&camera, (u32)&pRoot->mCamera[i]);
            pRoot->mpAnmScn->GetCamera(*(nw4r::g3d::Camera*)&camera, i);
        }
    }
}

/* 0x80083290 (0x4C): applies the bound scene animation, then runs the world pass over the children. */
extern "C" void scn_root_calc_world(ScnRoot* pRoot)
{
    scn_root_apply_anm_scn(pRoot);
    pRoot->G3dProc(ScnRoot::G3DPROC_CALC_WORLD, 0, NULL);
}

/* 0x800832DC (0x1C): runs the material pass over the children. */
extern "C" void scn_root_calc_material(ScnRoot* pRoot)
{
    pRoot->G3dProc(ScnRoot::G3DPROC_CALC_MAT, 0, NULL);
}

/* 0x800832F8 (0x30): constructs a vector and returns it. */
extern "C" nw4r::math::VEC3* vec3_ctor_return(nw4r::math::VEC3* pVec)
{
    VEC3_ctor(pVec);
    return pVec;
}

/* The math types' empty constructors, visible so the compiler knows the camera record's constructor does not throw
 * (their kept copies are 0x8005050C, 0x80075390 and 0x80043EA8). */
extern "C" inline void MTX34_ctor(nw4r::math::MTX34* out) {}
extern "C" inline void MTX44_ctor(nw4r::math::MTX44* pMtx) {}
extern "C" inline void VEC3_ctor(nw4r::math::VEC3* out) {}

/* 0x800834F0 (0x4): constructs an 8-byte vector (leaves it unset). */
/* untyped: opaque handle - callers pass their own 8-byte vector records */
extern "C" inline void VEC2_ctor(void* p)
{
}

/* G3dObj's empty operator delete, visible so the destructor's deleting tail does not throw. */
inline void nw4r::g3d::G3dObj::operator delete(void* pBlock) /* untyped: byte range */
{
}

/* 0x80083488 (0x68): constructs a camera record's matrix and vector members. */
inline CameraData::CameraData()
{
    MTX34_ctor((nw4r::math::MTX34*)mViewMtx);
    MTX44_ctor((nw4r::math::MTX44*)mProjMtx);
    VEC3_ctor((nw4r::math::VEC3*)&mPosX);
    VEC3_ctor((nw4r::math::VEC3*)&mTargetX);
    VEC3_ctor((nw4r::math::VEC3*)&mUpX);
    VEC3_ctor((nw4r::math::VEC3*)&mUnk98);
    VEC2_ctor(&mViewportX);
    VEC2_ctor(&mViewportW);
}

#pragma push
#pragma dont_inline on
/* 0x80083328 (0x11C): builds the root over its child array, draw collection and light buffers, and initialises
 * every camera and fog record. */
/* untyped: opaque handle - the draw collection, the light-object buffers */
ScnRoot::ScnRoot(MEMAllocator* pHeap, void* pCollection, ScnObj** ppChildren, u32 maxChildren, u32 numLightObj,
                 u32 numAmbLightObj, void* pLightObjBuf, void* pLightAnmBuf, void* pAmbLightObjBuf)
    : ScnGroup(pHeap, ppChildren, maxChildren), mpCollection(pCollection), mDrawMode(2), mScnRootFlags(0),
      mCurrentCameraID(0),
      mLightSetting((nw4r::g3d::LightObj*)pLightObjBuf, (nw4r::g3d::AmbLightObj*)pLightAnmBuf, numLightObj,
                    (nw4r::g3d::LightSetData*)pAmbLightObjBuf, numAmbLightObj)
{
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

/* 0x800834F4 (0xA4): detaches the scene animation and destroys the light setting. */
ScnRoot::~ScnRoot()
{
    if (mpAnmScn) {
        mpAnmScn->G3dProc(G3DPROC_DETACH_PARENT, 0, this);
    }
}
#pragma pop

/* 0x80083598 (0x160): culls the object against the gather frustum and appends it to the opaque and translucent
 * lists it asks for. */
IScnObjGather::CullingStatus ScnObjGather::Add(ScnObj* pObj, bool opa, bool xlu)
{
    CullingStatus status = CULLINGSTATUS_NOTCULLED;
    if (scnobj_culling_frustum != NULL) {
        u32 culling;
        pObj->GetScnObjOption(ScnObj::OPTION_ENABLE_CULLING, &culling);
        if (culling) {
            nw4r::math::AABB box;
            AABB_ctor(&box);
            pObj->GetBoundingVolume(ScnObj::BOUNDINGVOLUME_AABB_WORLD, &box);
            int result = scnobj_culling_frustum->IntersectAABB_Ex(&box);
            if (result == 0) {
                return CULLINGSTATUS_CULLED;
            }
            if (result == 1) {
                status = CULLINGSTATUS_NOTEST;
            }
        }
    }
    if (opa) {
        if (mNumScnObjOpa < mSizeScnObj) {
            mpArrayOpa[mNumScnObjOpa++] = pObj;
        } else {
            nw4r::db::Panic("g3d_scnroot.cpp", 0x287, "NW4R:Failed assertion 0");
            return status;
        }
    }
    if (xlu) {
        if (mNumScnObjXlu < mSizeScnObj) {
            mpArrayXlu[mNumScnObjXlu++] = pObj;
        } else {
            nw4r::db::Panic("g3d_scnroot.cpp", 0x295, "NW4R:Failed assertion 0");
            return status;
        }
    }
    return status;
}

/* The identity the swap moves its values through. */
template <class T>
T& scnroot_move(T& value)
{
    return value;
}

/* Exchanges the elements at `pA` and `pB`. */
template <class Iterator>
void scnroot_iter_swap(Iterator pA, Iterator pB)
{
    ScnObj* tmp = scnroot_move(*pA);
    *pA = scnroot_move(*pB);
    *pB = scnroot_move(tmp);
}

/* The first smallest element of [first, last). */
template <class Iterator, class Compare>
Iterator scnroot_min_element(Iterator first, Iterator last, Compare comp)
{
    Iterator min = first;
    if (first != last) {
        for (++first; first != last; ++first) {
            if (comp(*first, *min)) {
                min = first;
            }
        }
    }
    return min;
}

/* Sorts a short range by repeated selection of the smallest element. */
template <class Iterator, class Compare>
void scnroot_insertion_sort(Iterator first, Iterator last, Compare comp)
{
    if (first != last) {
        for (Iterator end = last - 1; first != end; ++first) {
            Iterator min = scnroot_min_element<Iterator, Compare>(first, last, comp);
            if (min != first) {
                scnroot_iter_swap(min, first);
            }
        }
    }
}

/* Moves the median of the three elements to `pC`. */
template <class Iterator, class Compare>
void scnroot_median3(Iterator pA, Iterator pB, Iterator pC, Compare comp)
{
    bool cNotBelowA = !comp(*pC, *pA);
    bool bNotBelowC = !comp(*pB, *pC);
    if (cNotBelowA && bNotBelowC) {
        return;
    }
    if (!cNotBelowA && !bNotBelowC) {
        scnroot_iter_swap(pA, pB);
        return;
    }
    if (comp(*pB, *pA)) {
        scnroot_iter_swap(pA, pB);
    }
    if (cNotBelowA) {
        scnroot_iter_swap(pB, pC);
    } else {
        scnroot_iter_swap(pA, pC);
    }
}

/* Quicksort over [first, last): selection sort up to 20 elements, a pseudo-random median-of-three pivot, the
 * smaller side sorted by recursion. */
template <class Iterator, class Compare>
void scnroot_sort(Iterator first, Iterator last, Compare comp)
{
    static int pivotState = -4;
    for (;;) {
        long len = last - first;
        if (len <= 1) {
            return;
        }
        if (len <= 20) {
            scnroot_insertion_sort<Iterator, nw4r::g3d::ScnObjLessThanFunc&>(first, last, comp);
            return;
        }
        Iterator pA = first + (len / 4 + pivotState % 5);
        if (++pivotState >= 5) {
            pivotState = -4;
        }
        long offsetB = (len * 3) / 4;
        Iterator pB = first + (offsetB + pivotState % 5);
        if (++pivotState >= 5) {
            pivotState = -4;
        }
        Iterator pivot = last - 1;
        scnroot_median3<Iterator, nw4r::g3d::ScnObjLessThanFunc&>(pA, pB, pivot, comp);
        Iterator j = pivot;
        Iterator i = first;
        while (comp(*i, *pivot)) {
            ++i;
        }
        while (i != --j && !comp(*j, *pivot)) {
        }
        if (i < j) {
            scnroot_iter_swap(i, j);
            ++i;
            for (;;) {
                while (comp(*i, *pivot)) {
                    ++i;
                }
                while (!comp(*--j, *pivot)) {
                }
                if (i >= j) {
                    break;
                }
                scnroot_iter_swap(i, j);
                ++i;
            }
        }
        if (i == first) {
            scnroot_iter_swap(i, pivot);
            ++i;
            j = last - 1;
            if (!comp(*first, *j)) {
                while (i != last && !comp(*first, *i)) {
                    ++i;
                }
                if (i < j) {
                    scnroot_iter_swap(i, j);
                }
            }
            if (i < j) {
                for (;;) {
                    while (!comp(*first, *i)) {
                        ++i;
                    }
                    while (comp(*first, *--j)) {
                    }
                    if (i >= j) {
                        break;
                    }
                    scnroot_iter_swap(i, j);
                    ++i;
                }
            }
            first = i;
            continue;
        }
        if (i - first < last - i) {
            scnroot_sort<Iterator, nw4r::g3d::ScnObjLessThanFunc&>(first, i, comp);
            first = i;
        } else {
            scnroot_sort<Iterator, nw4r::g3d::ScnObjLessThanFunc&>(i, last, comp);
            last = i;
        }
    }
}
/* 0x800836F8 (0x60): sorts both lists by draw priority, then view depth. */
void ScnObjGather::ZSort()
{
    scnroot_sort(mpArrayOpa, mpArrayOpa + mNumScnObjOpa, scnroot_less_z_opa);
    scnroot_sort(mpArrayXlu, mpArrayXlu + mNumScnObjXlu, scnroot_less_z_xlu);
}

/* 0x80083FDC (0xAC): translucent order: the lower priority first, then the farther object (smaller view z). */
extern "C" bool scnroot_less_z_xlu(ScnObj* pLhs, ScnObj* pRhs)
{
    int lhs = pLhs->GetPriorityDrawXlu();
    int rhs = pRhs->GetPriorityDrawXlu();
    if (lhs == rhs) {
        f32 rhsZ = pRhs->GetMtxPtr(ScnObj::MTX_VIEW)->m[2][3];
        return pLhs->GetMtxPtr(ScnObj::MTX_VIEW)->m[2][3] < rhsZ;
    }
    return lhs < rhs;
}

/* 0x80084088 (0x8): the translucent draw priority. */
int ScnObj::GetPriorityDrawXlu() const
{
    return mPriorityDrawXlu;
}

/* 0x80084090 (0xAC): opaque order: the lower priority first, then the nearer object (larger view z). */
extern "C" bool scnroot_less_z_opa(ScnObj* pLhs, ScnObj* pRhs)
{
    int lhs = pLhs->GetPriorityDrawOpa();
    int rhs = pRhs->GetPriorityDrawOpa();
    if (lhs == rhs) {
        f32 rhsZ = pRhs->GetMtxPtr(ScnObj::MTX_VIEW)->m[2][3];
        return pLhs->GetMtxPtr(ScnObj::MTX_VIEW)->m[2][3] > rhsZ;
    }
    return lhs < rhs;
}

/* 0x8008413C (0x8): the opaque draw priority. */
int ScnObj::GetPriorityDrawOpa() const
{
    return mPriorityDrawOpa;
}

/* 0x80084144 (0x60): sorts both lists by draw priority, then the objects' own sort values. */
void ScnObjGather::Sort()
{
    scnroot_sort(mpArrayOpa, mpArrayOpa + mNumScnObjOpa, scnroot_less_sort_opa);
    scnroot_sort(mpArrayXlu, mpArrayXlu + mNumScnObjXlu, scnroot_less_sort_xlu);
}

/* 0x800841A4 (0xAC): translucent order by priority, then GetValueForSortXlu. */
extern "C" bool scnroot_less_sort_xlu(ScnObj* pLhs, ScnObj* pRhs)
{
    int lhs = pLhs->GetPriorityDrawXlu();
    int rhs = pRhs->GetPriorityDrawXlu();
    if (lhs == rhs) {
        f32 rhsValue = pRhs->GetValueForSortXlu();
        return pLhs->GetValueForSortXlu() < rhsValue;
    }
    return lhs < rhs;
}

/* 0x80084250 (0xAC): opaque order by priority, then GetValueForSortOpa. */
extern "C" bool scnroot_less_sort_opa(ScnObj* pLhs, ScnObj* pRhs)
{
    int lhs = pLhs->GetPriorityDrawOpa();
    int rhs = pRhs->GetPriorityDrawOpa();
    if (lhs == rhs) {
        f32 rhsValue = pRhs->GetValueForSortOpa();
        return pLhs->GetValueForSortOpa() < rhsValue;
    }
    return lhs < rhs;
}

/* 0x800842FC (0x68): sorts both lists with the caller's predicates. */
void ScnObjGather::Sort(nw4r::g3d::ScnObjLessThanFunc pOpaFunc, nw4r::g3d::ScnObjLessThanFunc pXluFunc)
{
    scnroot_sort(mpArrayOpa, mpArrayOpa + mNumScnObjOpa, pOpaFunc);
    scnroot_sort(mpArrayXlu, mpArrayXlu + mNumScnObjXlu, pXluFunc);
}

/* 0x80084364 (0x80): draws the opaque list in order. */
void ScnObjGather::DrawOpa(u32* pDrawMode)
{
    u32 i = 0;
    while (i != mNumScnObjOpa) {
        mpArrayOpa[i++]->G3dProc(ScnObj::G3DPROC_DRAW_OPA, 0, pDrawMode);
    }
}

/* 0x800843E4 (0x80): draws the translucent list in order. */
void ScnObjGather::DrawXlu(u32* pDrawMode)
{
    u32 i = 0;
    while (i != mNumScnObjXlu) {
        mpArrayXlu[i++]->G3dProc(ScnObj::G3DPROC_DRAW_XLU, 0, pDrawMode);
    }
}

/* 0x80084464 (0x8): the lists are sorted by ZSort or Sort, never by the user. */
bool ScnObjGather::IsUserSorted() const
{
    return false;
}

/* 0x8008446C (0xC0): the collector over the caller's two list buffers of `bufSize` entries each. */
ScnObjGather::ScnObjGather(ScnObj** ppBufOpa, ScnObj** ppBufXlu, u32 bufSize)
    : mpArrayOpa(ppBufOpa), mpArrayXlu(ppBufXlu), mSizeScnObj(bufSize), mNumScnObjOpa(0), mNumScnObjXlu(0)
{
    if (ppBufOpa == NULL) {
        nw4r::db::Panic("g3d_scnroot.cpp", 0x2D3, "NW4R:Pointer must not be NULL (bufOpa)");
    }
    if (ppBufXlu == NULL) {
        nw4r::db::Panic("g3d_scnroot.cpp", 0x2D4, "NW4R:Pointer must not be NULL (bufXlu)");
    }
}

/* 0x8008452C (0x30): the ScnRoot type object. */
const nw4r::g3d::G3dObj::TypeObj ScnRoot::GetTypeObj() const
{
    const u8* local;

    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&local, (const u8*)&scn_typename_ScnRoot));
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

    return *reinterpret_cast<const TypeObj*>(type_obj_set_name_scnleaf(&local, (const u8*)&scn_typename_ScnRoot));
}
