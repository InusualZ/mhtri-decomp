/*
 * g3d/g3d_camera.cpp - nw4r g3d `Camera` posture/projection/viewport/scissor methods and the `ResCommon<CameraData>`
 *   accessors.
 * RANGE. .text 0x800746DC-0x80075E9C (30 functions); extab, extabindex, .data 0x8058E430-0x8058E570 (opens on
 *   "g3d_camera.cpp", read from fn_800746DC on), .sdata2 0x80795DC8-0x80795E00.  The six accessors
 *   res_mdl_info_num_view_mtx..fn_800746D4 before it (no data references) sit in `g3d/g3d_calcworld.cpp` by the candidate cut.
 *   The tail 0x80075DCC-0x80075E9C (sin_cos_deg, fn_80075DD8, fn_80075E98) is this TU's: fn_80075DD8 is called only
 *   from camera_get_projection_mtx/camera_gx_set_projection, sin_cos_deg first from fn_80075940, and the pool words both read (0x80795DF8 from
 *   fn_80075940, 0x80795DFC from sin_cos_deg) end this unit's `.sdata2`; `g3d/g3d_dcc.cpp`'s text opens on its
 *   "g3d_dcc.cpp" referrer fn_80075E9C.
 * NAMES. camera_init is a GUESS (the evidence follows).  sin_cos_deg is a GUESS (nw4r::math::SinCosDeg: the 256/360
 *   table-index scale, then the pointer pair passed through).  camera_ref and camera_ref_const are GUESSES (the
 *   ResCommon<CameraData> ref/ref const copies, g3d_rescommon_ac.h:140/143); camera_get_viewport,
 *   camera_get_projection_mtx and camera_gx_set_viewport/_projection/_scissor/_scissor_box_offset are GUESSES (nw4r's
 *   Camera::GetViewport, GetProjectionMtx and GXSetViewport/GXSetProjection/GXSetScissor/GXSetScissorBoxOffset by
 *   their bodies); mtx44_ptr is a GUESS (0x80075844: the MTX44 pointer conversion GXSetProjection reads through).
 *   Map stems, plus the `nw4r::g3d::Camera` members the map carries mangled (`SetPosition`, `SetPosture`,
 *   `SetPerspective`, `GetCameraMtx`); MTX44_ctor is a GUESS (0x80075390: the empty 4x4 matrix constructor).
 * RESIDUALS. None in `.text`; flipcheck READY and a trial flip keeps the DOL hash (with the camera renames applied).
 *   The remaining `fn_` stems in this file are the unit's own rows not renamed yet and foreign rows (fn_80067EE8,
 *   fn_800508A8, mtx34_get_ptr's neighbours).
 * SHAPES. File-scope `#pragma peephole off` (retail's unfused forms; playbook idea 106) and `#pragma fp_contract
 *   off`.  Definitions follow retail's address order; camera_ref_const and camera_ref are `inline` under
 *   `#pragma dont_inline on` (weak copies after their first referencer, their `@STRING@` tail after the TU's own
 *   strings; ref const is defined first so each copy cites its own strings).  GXSetViewportJitter and GXProject are
 *   declared in the SDK's argument order (floats first); the guards read `if (p != NULL && IsValid())`; the
 *   "Unknown camera type" panic passes the flags word (it is what puts the flags in r6).
 */


#include "types.h"
#include "nw4r/fn_805012C4.h" /* nw4r::math::MTX44Copy (rule 2) */
#include "nw4r/math.h"
#include "g3d/g3d_calcview.h" /* mtx34_copy_ps (rule 2) */
#include "g3d/g3d_state.h" /* g3d_state_get_render_mode (rule 2) */
#include "MTX/mtx.h" /* C_MTXLookAt / C_MTXLight* (rule 2) */
#include "MTX/mtx44.h" /* C_MTXOrtho / C_MTXFrustum / C_MTXPerspective (rule 2) */
#include "nw4r/math_triangular.h" /* math_sincos_idx (rule 2) */
#include "main.h"         /* fn_8004028C/fn_8004029C, owned by main.cpp (rule 2) */
#include "g3d/g3d_camera_types.h"
#include "g3d/g3d_anmscn.h" /* fn_80067EE8, owned by g3d/g3d_anmscn.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

#pragma pool_data off

#pragma peephole off

/* The target object contains no fused multiply-add at all while `cflags_g3d` passes
 * `-fp_contract on`, so the original file carried the pragma. File-scoped (see header). */
#pragma fp_contract off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace db


struct RenderModeObj;
struct ScissorSize;

/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)            */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void GXSetProjection(const f32* pMtx, s32 type);
extern "C" void GXSetScissor(u32 left, u32 top, u32 width, u32 height);
extern "C" void GXSetScissorBoxOffset(s32 x, s32 y);
extern "C" void GXSetViewport(f32 x, f32 y, f32 width, f32 height, f32 near, f32 far);
extern "C" void mtx34_identity(void* pOut);
extern "C" void* mtx34_get_ptr(void* pMtx);
extern "C" void vec3_normalize_into(void* pOut, const void* pIn);
extern "C" s32 vec3_to_vec(const void* pIn);
extern "C" void vec3_cross(void* pOut, const void* pA, const void* pB);
extern "C" f32 vec3_dot(const void* pA, const void* pB);
extern "C" void GXProject(f32 x, f32 y, f32 z, const f32 (*pViewMtx)[4], const f32* pParams, const f32* pViewport,
                          f32* pScreenX, f32* pScreenY, f32* pScreenZ);
extern "C" void GXSetViewportJitter(f32 left, f32 top, f32 width, f32 height, f32 nearZ, f32 farZ, u32 field);


/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The render-mode object `g3d_state_get_render_mode` returns (`GXRenderModeObj`-shaped). */
struct RenderModeObj {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u16 mUnk04;
    /* +0x06 */ u16 mUnk06;
    /* +0x08 */ u16 mUnk08;
    /* +0x0A */ u16 mUnk0A;
    /* +0x0C */ u16 mUnk0C;
    /* +0x0E */ u16 mUnk0E;
    /* +0x10 */ u16 mUnk10;
    /* +0x12 */ u16 mUnk12;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u8 mUnk18;
}; /* size: 0x1C */

/* The scissor-size source `fn_8004028C` returns. */
struct ScissorSize {
    /* +0x0 */ u16 mWidth;
    /* +0x2 */ u16 mHeight;
}; /* size: 0x4 */

/* The seven-float projection parameter block `GXProject` reads at its second argument. */
struct ProjParams {
    /* +0x00 */ f32 mProjType;
    /* +0x04 */ f32 mUnk04;
    /* +0x08 */ f32 mUnk08;
    /* +0x0C */ f32 mUnk0C;
    /* +0x10 */ f32 mUnk10;
    /* +0x14 */ f32 mUnk14;
    /* +0x18 */ f32 mUnk18;
}; /* size: 0x1C */


/* `nw4r::g3d::Camera` itself lives in `g3d/g3d_camera_types.h` (rule 1: its consumers include the same one). */

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

CameraData* camera_ref(nw4r::g3d::Camera* pSelf);
CameraData* camera_ref_const(const nw4r::g3d::Camera* pSelf);
void camera_init(nw4r::g3d::Camera* pSelf);
void fn_80074758(nw4r::g3d::Camera* pSelf, u16 a1, u16 a2, u16 a3, u16 a4, u16 a5, u16 a6);
void camera_get_position(nw4r::g3d::Camera* pSelf, nw4r::math::VEC3* pOut);
void fn_80074AA8(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pZ);
s32 fn_80074D38(const f32* pA, const f32* pB);
void camera_posture_get(nw4r::g3d::Camera* pSelf, nw4r::g3d::Camera::PostureInfo* pOut);
void fn_80074FEC(nw4r::g3d::Camera* pSelf, u32 x, u32 y, u32 w, u32 h);
void camera_set_viewport(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d);
void camera_get_viewport(nw4r::g3d::Camera* pSelf, f32* p1, f32* p2, f32* p3, f32* p4, f32* p5, f32* p6);
void camera_world_to_screen(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec);
void MTX44_ctor(nw4r::math::MTX44* pMtx);
void camera_get_projection_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX44* pOut);
void camera_get_projection_tex_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX34* pOut);
void camera_get_environment_tex_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX34* pOut);
void camera_gx_set_viewport(nw4r::g3d::Camera* pSelf);
void camera_gx_set_projection(nw4r::g3d::Camera* pSelf);
f32* mtx44_ptr(f32 (*pMtx)[4]);
void camera_gx_set_scissor(nw4r::g3d::Camera* pSelf);
void camera_gx_set_scissor_box_offset(nw4r::g3d::Camera* pSelf);
void fn_80075940(nw4r::g3d::Camera* pSelf);
void sin_cos_deg(f32* pOutSin, f32* pOutCos, f32 angle);
void fn_80075DD8(nw4r::g3d::Camera* pSelf);
f32 (*fn_80075E98(f32 (*pMtx)[4]))[4];

/* ------------------------------------------------------------------------------------------------ */
/* g3d_camera.cpp                                                                                    */
/* ------------------------------------------------------------------------------------------------ */

#pragma dont_inline on
/* The same accessor's second instantiation (g3d_rescommon_ac.h:143). */
inline CameraData* camera_ref_const(const nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 143, "NW4R:Pointer must not be NULL (mpData)");
    }
    return pSelf->mpData;
}

/* The `ResCommon<CameraData>::ref()`-style accessor (g3d_rescommon_ac.h:140). */
inline CameraData* camera_ref(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 140, "NW4R:Pointer must not be NULL (mpData)");
    }
    return pSelf->mpData;
}

/* Reset every camera field from the current render mode. */
void camera_init(nw4r::g3d::Camera* pSelf) {
    RenderModeObj* pMode = (RenderModeObj*)g3d_state_get_render_mode();
    if (pMode == NULL) {
        nw4r::db::Panic("g3d_camera.cpp", 50, "NW4R:Pointer must not be NULL (rmode)");
    }
    u16 width = pMode->mUnk04;
    fn_80074758(pSelf, width, pMode->mUnk06, width, pMode->mUnk08, pMode->mUnk0E, pMode->mUnk10);
}

void fn_80074758(nw4r::g3d::Camera* pSelf, u16 a1, u16 a2, u16 a3, u16 a4, u16 a5, u16 a6) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 62, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    ScissorSize* pSize = (ScissorSize*)fn_8004028C();
    CameraData* pData = camera_ref(pSelf);
    pData->mFlags = 0x21;
    pData->mPosX = 0.0f;
    pData->mPosY = 0.0f;
    pData->mPosZ = 15.0f;
    pData->mTargetX = 0.0f;
    pData->mTargetY = 1.0f;
    pData->mTargetZ = 0.0f;
    pData->mUpX = 0.0f;
    pData->mUpY = 0.0f;
    pData->mUpZ = 0.0f;
    pData->mUnk98 = 0.0f;
    pData->mUnk9C = 0.0f;
    pData->mUnkA0 = 0.0f;
    pData->mUnkA4 = 0.0f;
    pData->mProjType = 0;
    pData->mProjA = 60.0f;
    pData->mProjB = screen_aspect_get();
    pData->mProjC = 0.1f;
    pData->mProjD = 1000.0f;
    pData->mUnkBC = 0.0f;
    pData->mUnkC0 = 448.0f;
    pData->mUnkC4 = 0.0f;
    pData->mUnkC8 = (f32)a5;
    pData->mUnkCC = 0.5f;
    pData->mUnkD0 = 0.5f;
    pData->mUnkD4 = 0.5f;
    pData->mUnkD8 = 0.5f;
    pData->mViewportX = 0.0f;
    pData->mViewportY = 0.0f;
    pData->mViewportW = (f32)a3;
    pData->mViewportH = 448.0f;
    pData->mViewportNear = 0.0f;
    pData->mViewportFar = 1.0f;
    pData->mScissorX = 0;
    pData->mScissorY = 0;
    pData->mScissorW = pSize->mWidth;
    pData->mScissorH = pSize->mHeight;
    pData->mScissorOffsetX = 0;
    pData->mScissorOffsetY = 0;
}

void nw4r::g3d::Camera::SetPosition(const math::VEC3& rPos) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 180, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = camera_ref(this);
        copyVec3((nw4r::math::VEC3*)&pData->mPosX, (const nw4r::math::VEC3*)&rPos);
        pData->mFlags &= ~8;
    }
}

void camera_get_position(nw4r::g3d::Camera* pSelf, nw4r::math::VEC3* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 195, "NW4R:Failed assertion IsValid()");
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        /* `mPosX/mPosY/mPosZ` are three consecutive floats of the camera record, so the vector
         * is the address of the first of them (the engine's own view; rule 11). */
        copyVec3(pOut, (const nw4r::math::VEC3*)&pData->mPosX);
    }
}

void fn_80074AA8(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pZ) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 206, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        if (pX != NULL) {
            *pX = pData->mPosX;
        }
        if (pY != NULL) {
            *pY = pData->mPosY;
        }
        if (pZ != NULL) {
            *pZ = pData->mPosZ;
        }
    }
}

void nw4r::g3d::Camera::SetPosture(const PostureInfo& rInfo) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 226, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(this)) {
        return;
    }
    CameraData* pData = camera_ref(this);
    switch (rInfo.mType) {
    case 0: {
        if ((pData->mFlags & 1) == 0 || fn_80074D38(&rInfo.mPosX, &pData->mTargetX) != 0 ||
            fn_80074D38(&rInfo.mTargetX, &pData->mUpX) != 0) {
            pData->mFlags &= ~7;
            pData->mFlags |= 1;
            copyVec3((nw4r::math::VEC3*)&pData->mTargetX, (const nw4r::math::VEC3*)&rInfo.mPosX);
            copyVec3((nw4r::math::VEC3*)&pData->mUpX, (const nw4r::math::VEC3*)&rInfo.mTargetX);
            pData->mFlags &= ~8;
        }
        break;
    }
    case 1: {
        if ((pData->mFlags & 2) == 0 || fn_80074D38(&rInfo.mUpX, &pData->mUnk98) != 0) {
            pData->mFlags &= ~7;
            pData->mFlags |= 2;
            copyVec3((nw4r::math::VEC3*)&pData->mUnk98, (const nw4r::math::VEC3*)&rInfo.mUpX);
            pData->mFlags &= ~8;
        }
        break;
    }
    case 2: {
        if ((pData->mFlags & 4) == 0 || fn_80074D38(&rInfo.mTargetX, &pData->mUpX) != 0 ||
            rInfo.mUnk28 != pData->mUnkA4) {
            pData->mFlags &= ~7;
            pData->mFlags |= 4;
            copyVec3((nw4r::math::VEC3*)&pData->mUpX, (const nw4r::math::VEC3*)&rInfo.mTargetX);
            pData->mUnkA4 = rInfo.mUnk28;
            pData->mFlags &= ~8;
        }
        break;
    }
    }
}

/* True when the two three-float vectors differ. */
s32 fn_80074D38(const f32* pA, const f32* pB) {
    s32 result = 0;
    if (pA[0] != pB[0] || pA[1] != pB[1] || pA[2] != pB[2]) {
        result = 1;
    }
    return result;
}

void camera_posture_get(nw4r::g3d::Camera* pSelf, nw4r::g3d::Camera::PostureInfo* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 282, "NW4R:Failed assertion IsValid()");
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        u32 flags = pData->mFlags;
        if ((flags & 1) != 0) {
            pOut->mType = 0;
            copyVec3((nw4r::math::VEC3*)&pOut->mPosX, (const nw4r::math::VEC3*)&pData->mTargetX);
            copyVec3((nw4r::math::VEC3*)&pOut->mTargetX, (const nw4r::math::VEC3*)&pData->mUpX);
            return;
        }
        if ((flags & 2) != 0) {
            pOut->mType = 1;
            copyVec3((nw4r::math::VEC3*)&pOut->mUpX, (const nw4r::math::VEC3*)&pData->mUnk98);
            return;
        }
        pOut->mType = 2;
        copyVec3((nw4r::math::VEC3*)&pOut->mTargetX, (const nw4r::math::VEC3*)&pData->mUpX);
        pOut->mUnk28 = pData->mUnkA4;
    }
}

void nw4r::g3d::Camera::SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 329, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = camera_ref(this);
        pData->mProjType = 0;
        pData->mProjA = fovy;
        pData->mProjB = aspect;
        pData->mProjC = near;
        pData->mProjD = far;
        pData->mFlags &= ~0xF0;
        pData->mFlags |= 0x20;
    }
}

void nw4r::g3d::Camera::SetOrtho(f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 371, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = camera_ref(this);
        pData->mProjType = 1;
        pData->mUnkBC = top;
        pData->mUnkC0 = bottom;
        pData->mUnkC4 = left;
        pData->mUnkC8 = right;
        pData->mProjC = near;
        pData->mProjD = far;
        pData->mFlags &= ~0xF0;
        pData->mFlags |= 0x40;
    }
}

void fn_80074FEC(nw4r::g3d::Camera* pSelf, u32 x, u32 y, u32 w, u32 h) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 430, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref(pSelf);
        pData->mScissorX = x;
        pData->mScissorY = y;
        pData->mScissorW = w;
        pData->mScissorH = h;
    }
}

void camera_set_viewport(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 463, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref(pSelf);
        pData->mViewportX = a;
        pData->mViewportY = b;
        pData->mViewportW = c;
        pData->mViewportH = d;
        fn_80074FEC(pSelf, (u32)a, (u32)b, (u32)c, (u32)d);
    }
}

void camera_get_viewport(nw4r::g3d::Camera* pSelf, f32* p1, f32* p2, f32* p3, f32* p4, f32* p5, f32* p6) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 521, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        if (p1 != NULL) {
            *p1 = pData->mViewportX;
        }
        if (p2 != NULL) {
            *p2 = pData->mViewportY;
        }
        if (p3 != NULL) {
            *p3 = pData->mViewportW;
        }
        if (p4 != NULL) {
            *p4 = pData->mViewportH;
        }
        if (p5 != NULL) {
            *p5 = pData->mViewportNear;
        }
        if (p6 != NULL) {
            *p6 = pData->mViewportFar;
        }
    }
}

void camera_world_to_screen(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec) {
    if (pOut == NULL) {
        return;
    }
    f32 viewMtx[12];
    f32 projMtx[16];
    f32 frustum[6];
    ProjParams params;
    MTX34_ctor((nw4r::math::MTX34*)viewMtx);
    MTX44_ctor((nw4r::math::MTX44*)projMtx);
    pSelf->GetCameraMtx((nw4r::math::MTX34*)viewMtx);
    camera_get_projection_mtx(pSelf, (nw4r::math::MTX44*)projMtx);
    CameraData* pData = camera_ref_const(pSelf);
    f32 projType = (f32)pData->mProjType;
    params.mProjType = projType;
    params.mUnk04 = projMtx[0];
    params.mUnk0C = projMtx[5];
    params.mUnk14 = projMtx[10];
    params.mUnk18 = projMtx[11];
    if (1.0f == projType) {
        params.mUnk08 = projMtx[3];
        params.mUnk10 = projMtx[7];
    } else {
        params.mUnk08 = projMtx[2];
        params.mUnk10 = projMtx[6];
    }
    camera_get_viewport(pSelf, &frustum[0], &frustum[1], &frustum[2], &frustum[3], &frustum[4], &frustum[5]);
    nw4r::math::VEC3* pScreen = (nw4r::math::VEC3*)pOut;
    GXProject(pVec->x, pVec->y, pVec->z, (const f32 (*)[4])mtx34_get_ptr(viewMtx), &params.mProjType, frustum,
              &pScreen->x, &pScreen->y, &pScreen->z);
}

/* 0x80075390 (0x4): constructs a 4x4 matrix (nothing to initialise). */
void MTX44_ctor(nw4r::math::MTX44* pMtx) {
}

void nw4r::g3d::Camera::GetCameraMtx(math::MTX34* pMtx) const {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 629, "NW4R:Failed assertion IsValid()");
    }
    if (pMtx != NULL && fn_80067EE8(this)) {
        CameraData* pData = camera_ref_const((nw4r::g3d::Camera*)this);
        if ((pData->mFlags & 8) == 0) {
            fn_80075940((nw4r::g3d::Camera*)this);
        }
        mtx34_copy_ps((Mtx34*)pMtx, (const Mtx34*)pData);
    }
}

void camera_get_projection_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX44* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 647, "NW4R:Failed assertion IsValid()");
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        if ((pData->mFlags & 0x80) == 0) {
            fn_80075DD8((nw4r::g3d::Camera*)pSelf);
        }
        nw4r::math::MTX44Copy(pOut, (const nw4r::math::MTX44*)&pData->mProjMtx);
    }
}

void camera_get_projection_tex_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX34* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 665, "NW4R:Failed assertion IsValid()");
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        u32 flags = pData->mFlags;
        if ((flags & 0x40) != 0) {
            C_MTXLightOrtho((f32 (*)[4])mtx34_get_ptr(pOut), pData->mUnkBC, pData->mUnkC0, pData->mUnkC4,
                            pData->mUnkC8, pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
            return;
        }
        if ((flags & 0x10) != 0) {
            C_MTXLightFrustum((f32 (*)[4])mtx34_get_ptr(pOut), pData->mUnkBC, pData->mUnkC0, pData->mUnkC4,
                              pData->mUnkC8, pData->mProjC, pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4,
                              pData->mUnkD8);
            return;
        }
        C_MTXLightPerspective((f32 (*)[4])mtx34_get_ptr(pOut), pData->mProjA, pData->mProjB, pData->mUnkCC,
                              -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
    }
}

void camera_get_environment_tex_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX34* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 713, "NW4R:Failed assertion IsValid()");
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = camera_ref_const(pSelf);
        mtx34_identity(pOut);
        pOut->m[0][0] = pData->mUnkCC;
        pOut->m[0][3] = pData->mUnkD4;
        pOut->m[1][1] = -pData->mUnkD0;
        pOut->m[1][3] = pData->mUnkD8;
        pOut->m[2][2] = 0.0f;
        pOut->m[2][3] = 1.0f;
    }
}

void camera_gx_set_viewport(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 736, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = camera_ref_const(pSelf);
    if (g3d_state_get_render_mode()->mUnk18 != 0) {
        GXSetViewportJitter(pData->mViewportX, pData->mViewportY, pData->mViewportW, pData->mViewportH,
                            pData->mViewportNear, pData->mViewportFar, (pData->mFlags & 0x100) != 0);
        return;
    }
    GXSetViewport(pData->mViewportX, pData->mViewportY, pData->mViewportW, pData->mViewportH,
                  pData->mViewportNear, pData->mViewportFar);
}

void camera_gx_set_projection(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 773, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = camera_ref_const(pSelf);
    if ((pData->mFlags & 0x80) == 0) {
        fn_80075DD8(pSelf);
    }
    GXSetProjection(mtx44_ptr(pData->mProjMtx), pData->mProjType);
}

/* 0x80075844 (0x4): the matrix as its first element's address (the MTX44 pointer conversion). */
f32* mtx44_ptr(f32 (*pMtx)[4]) {
    return pMtx[0];
}

void camera_gx_set_scissor(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 789, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = camera_ref_const(pSelf);
    GXSetScissor(pData->mScissorX, pData->mScissorY, pData->mScissorW, pData->mScissorH);
}

void camera_gx_set_scissor_box_offset(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 806, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = camera_ref_const(pSelf);
    GXSetScissorBoxOffset(pData->mScissorOffsetX, pData->mScissorOffsetY);
}


/* Rebuild the camera's view matrix from the posture the flags word selects: the +1 branch uses the
 * three-node path, +4 the position/target/up path and +2 the three-angle (Euler) path. */
void fn_80075940(nw4r::g3d::Camera* pSelf) {
    CameraData* pData = camera_ref_const(pSelf);
    u32 flags = pData->mFlags;
    nw4r::math::VEC3 diff;
    nw4r::math::VEC3 axisA;
    nw4r::math::VEC3 axisB;
    nw4r::math::VEC3 rowA;
    nw4r::math::VEC3 rowB;
    nw4r::math::VEC3 row0;
    nw4r::math::VEC3 row1;
    nw4r::math::VEC3 row2;
    f32 rotSin, rotCos;
    f32 sinX, sinY, sinZ, cosX, cosY, cosZ;
    if ((flags & 1) != 0) {
        if (fn_80074D38(&pData->mPosX, &pData->mUpX) == 0) {
            nw4r::db::Panic("g3d_camera.cpp", 832, "nw4r::g3d::Camera::UpdateCameraMtx:\ncamera position and target are the same(%f %f %f).", pData->mPosX, pData->mPosY,
                            pData->mPosZ);
        }
        C_MTXLookAt((f32 (*)[4])mtx34_get_ptr(pData), (const Vec*)vec3_to_vec(&pData->mPosX),
                    (const Vec*)vec3_to_vec(&pData->mTargetX), (const Vec*)vec3_to_vec(&pData->mUpX));
    } else if ((flags & 4) != 0) {
        nw4r::math::VEC3* pPos = (nw4r::math::VEC3*)&pData->mPosX;
        setVec3(&diff, pPos->x - pData->mUpX, pPos->y - pData->mUpY,
                    pPos->z - pData->mUpZ);
        if (0.0f == diff.x && 0.0f == diff.z) {
            pData->mViewMtx[0][0] = 1.0f;
            pData->mViewMtx[0][1] = 0.0f;
            pData->mViewMtx[0][2] = 0.0f;
            pData->mViewMtx[0][3] = -pPos->x;
            pData->mViewMtx[1][0] = 0.0f;
            pData->mViewMtx[1][1] = 0.0f;
            pData->mViewMtx[2][0] = 0.0f;
            pData->mViewMtx[2][2] = 0.0f;
            if (diff.y <= 0.0f) {
                pData->mViewMtx[1][2] = 1.0f;
                pData->mViewMtx[1][3] = -pPos->z;
                pData->mViewMtx[2][1] = -1.0f;
                pData->mViewMtx[2][3] = pPos->y;
            } else {
                pData->mViewMtx[1][2] = -1.0f;
                pData->mViewMtx[1][3] = pPos->z;
                pData->mViewMtx[2][1] = 1.0f;
                pData->mViewMtx[2][3] = -pPos->y;
            }
        } else {
            setVec3(&axisA, diff.z, 0.0f, -diff.x);
            VEC3_ctor(&axisB);
            vec3_normalize_into(&diff, &diff);
            vec3_normalize_into(&axisA, &axisA);
            vec3_cross(&axisB, &diff, &axisA);
            sin_cos_deg(&rotSin, &rotCos, pData->mUnkA4);
            VEC3_ctor(&rowA);
            VEC3_ctor(&rowB);
            rowA.x = rotSin * axisB.x + rotCos * axisA.x;
            rowA.y = rotSin * axisB.y;
            rowA.z = rotSin * axisB.z + rotCos * axisA.z;
            rowB.x = rotCos * axisB.x - rotSin * axisA.x;
            rowB.y = rotCos * axisB.y;
            rowB.z = rotCos * axisB.z - rotSin * axisA.z;
            pData->mViewMtx[0][0] = rowA.x;
            pData->mViewMtx[0][1] = rowA.y;
            pData->mViewMtx[0][2] = rowA.z;
            pData->mViewMtx[0][3] = -vec3_dot(pPos, &rowA);
            pData->mViewMtx[1][0] = rowB.x;
            pData->mViewMtx[1][1] = rowB.y;
            pData->mViewMtx[1][2] = rowB.z;
            pData->mViewMtx[1][3] = -vec3_dot(pPos, &rowB);
            pData->mViewMtx[2][0] = diff.x;
            pData->mViewMtx[2][1] = diff.y;
            pData->mViewMtx[2][2] = diff.z;
            pData->mViewMtx[2][3] = -vec3_dot(pPos, &diff);
        }
    } else {
        if ((flags & 2) == 0) {
            nw4r::db::Panic("g3d_camera.cpp", 926, "Unknown camera type(%x)", flags);
        }
        nw4r::math::VEC3* pPos = (nw4r::math::VEC3*)&pData->mPosX;
        sin_cos_deg(&sinX, &cosX, pData->mUnk98);
        sin_cos_deg(&sinY, &cosY, pData->mUnk9C);
        sin_cos_deg(&sinZ, &cosZ, pData->mUnkA0);
        VEC3_ctor(&row0);
        VEC3_ctor(&row1);
        VEC3_ctor(&row2);
        row0.x = sinZ * (sinX * sinY) + cosY * cosZ;
        row0.y = cosX * sinZ;
        row0.z = sinZ * (sinX * cosY) - sinY * cosZ;
        row1.x = cosZ * (sinX * sinY) - cosY * sinZ;
        row1.y = cosX * cosZ;
        row1.z = cosZ * (sinX * cosY) + sinY * sinZ;
        row2.x = cosX * sinY;
        row2.y = -sinX;
        row2.z = cosX * cosY;
        pData->mViewMtx[0][0] = row0.x;
        pData->mViewMtx[0][1] = row0.y;
        pData->mViewMtx[0][2] = row0.z;
        pData->mViewMtx[0][3] = -vec3_dot(pPos, &row0);
        pData->mViewMtx[1][0] = row1.x;
        pData->mViewMtx[1][1] = row1.y;
        pData->mViewMtx[1][2] = row1.z;
        pData->mViewMtx[1][3] = -vec3_dot(pPos, &row1);
        pData->mViewMtx[2][0] = row2.x;
        pData->mViewMtx[2][1] = row2.y;
        pData->mViewMtx[2][2] = row2.z;
        pData->mViewMtx[2][3] = -vec3_dot(pPos, &row2);
    }
    pData->mFlags |= 8;
}


} /* extern "C" */


extern "C" {

/* 0x80075DCC (0xC): the sine and cosine of `angle` degrees (nw4r's SinCosDeg: the 256-step table index is
 * `angle * 256 / 360`); the pointer pair passes through to math_sincos_idx untouched. */
void sin_cos_deg(f32* pOutSin, f32* pOutCos, f32 angle) {
    math_sincos_idx(pOutSin, pOutCos, (256.0f / 360.0f) * angle);
}

/* 0x80075DD8 (0xC0): rebuilds the projection matrix from the camera's orthographic, frustum or perspective
 * parameters and marks it current. */
void fn_80075DD8(nw4r::g3d::Camera* pSelf) {
    CameraData* pData = camera_ref_const(pSelf);

    if (pData->mFlags & 0x40) {
        C_MTXOrtho(fn_80075E98(pData->mProjMtx), pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8,
                   pData->mProjC, pData->mProjD);
    } else if (pData->mFlags & 0x10) {
        C_MTXFrustum(fn_80075E98(pData->mProjMtx), pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8,
                     pData->mProjC, pData->mProjD);
    } else {
        C_MTXPerspective(fn_80075E98(pData->mProjMtx), pData->mProjA, pData->mProjB, pData->mProjC,
                         pData->mProjD);
    }
    pData->mFlags |= 0x80;
}

/* 0x80075E98 (0x4): hands the 4x4 projection matrix back as its row pointer (the MTX44 conversion). */
f32 (*fn_80075E98(f32 (*pMtx)[4]))[4] {
    return pMtx;
}

} /* extern "C" */
