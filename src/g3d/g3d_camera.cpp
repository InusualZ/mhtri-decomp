/*
 * g3d/g3d_camera.cpp - nw4r g3d `Camera` posture/projection/viewport/scissor methods and the `ResCommon<CameraData>`
 *   accessors.
 * RANGE. .text 0x800746DC-0x80075E9C (30 functions); extab, extabindex, .data 0x8058E430-0x8058E570 (opens on
 *   "g3d_camera.cpp", read from fn_800746DC on), .sdata2 0x80795DC8-0x80795E00.  The six accessors
 *   res_mdl_info_num_view_mtx..fn_800746D4 before it (no data references) sit in `g3d/g3d_calcworld.cpp` by the candidate cut.
 *   The tail 0x80075DCC-0x80075E9C (sin_cos_deg, fn_80075DD8, fn_80075E98) is this TU's: fn_80075DD8 is called only
 *   from fn_80075440/fn_800757A8, sin_cos_deg first from fn_80075940, and the pool words both read (0x80795DF8 from
 *   fn_80075940, 0x80795DFC from sin_cos_deg) end this unit's `.sdata2`; `g3d/g3d_dcc.cpp`'s text opens on its
 *   "g3d_dcc.cpp" referrer fn_80075E9C.
 * NAMES. camera_init is a GUESS (the evidence follows).  sin_cos_deg is a GUESS (nw4r::math::SinCosDeg: the 256/360
 *   table-index scale, then the pointer pair passed through).
 *   Map stems, plus the three `nw4r::g3d::Camera` members the map carries mangled (`SetPosition`, `SetPosture`,
 *   `SetPerspective`, `GetCameraMtx`); MTX44_ctor is a GUESS (0x80075390: the empty 4x4 matrix constructor, the MTX34_ctor scheme).
 * RESIDUALS. Partial (15): fn_80074758, `Camera::SetPosition`, `Camera::SetPosture`, 0x80074D78-0x80074FEC (three),
 *   fn_8007507C, fn_80075258, 0x80075394-0x80075844 (six, `Camera::GetCameraMtx` first), fn_80075940.
 *   flipcheck: `.text` 0x16A0 of 0x16F0; `.data`: the strings are literals, the `_ac.h` accessor strings are not the inline tail yet (retail: weak `@STRING@` copies, one per inline
 *   instance, after the TU's own strings, playbook 80); `.sdata2` differs (0x80795DFC is declared, not pooled).
 * SHAPES. `#pragma peephole off` around fn_80075940 (playbook 32); file-scope `#pragma fp_contract off`.
 *   The unit compiles with `#pragma peephole off` (retail's unfused forms; playbook idea 106) except fn_800756DC,
 *   which measure better with the pass on.
 */


#include "types.h"
#include "nw4r/fn_805012C4.h" /* nw4r::math::MTX44Copy (rule 2) */
#include "nw4r/math.h"
#include "g3d/g3d_calcview.h" /* mtx34_copy_ps (rule 2) */
#include "g3d/g3d_state.h" /* g3d_state_get_render_mode (rule 2) */
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
extern "C" void GXProject(const f32* pViewMtx, const f32* pParams, const f32* pFrustum, void* pA,
                          void* pB, void* pC, f32 x, f32 y, f32 z);
extern "C" void GXSetViewportJitter(s32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g);
extern "C" void fn_804C64E0(s32 a, s32 b, s32 c);
extern "C" void fn_804C6660(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h, f32 i);
extern "C" void fn_804C6710(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
extern "C" void fn_804C6810(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h);

extern f32 lbl_80795DFC;

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

CameraData* fn_800748E4(nw4r::g3d::Camera* pSelf);
CameraData* fn_80074A54(nw4r::g3d::Camera* pSelf);
void camera_init(nw4r::g3d::Camera* pSelf);
void fn_80074758(nw4r::g3d::Camera* pSelf, u16 a1, u16 a2, u16 a3, u16 a4, u16 a5, u16 a6);
void fn_800749C8(nw4r::g3d::Camera* pSelf, nw4r::math::VEC3* pOut);
void fn_80074AA8(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pZ);
s32 fn_80074D38(const f32* pA, const f32* pB);
void fn_80074D78(nw4r::g3d::Camera* pSelf, nw4r::g3d::Camera::PostureInfo* pOut);
void fn_80074F24(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
void fn_80074FEC(nw4r::g3d::Camera* pSelf, u32 x, u32 y, u32 w, u32 h);
void fn_8007507C(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d);
void fn_80075170(nw4r::g3d::Camera* pSelf, f32* p1, f32* p2, f32* p3, f32* p4, f32* p5, f32* p6);
void fn_80075258(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec);
void MTX44_ctor(nw4r::math::MTX44* pMtx);
void camera_get_projection_mtx(nw4r::g3d::Camera* pSelf, void* pOut);
void camera_get_projection_tex_mtx(nw4r::g3d::Camera* pSelf, void* pOut);
void camera_get_environment_tex_mtx(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_800756DC(nw4r::g3d::Camera* pSelf);
void fn_800757A8(nw4r::g3d::Camera* pSelf);
void fn_80075844(void* pMtx);
void fn_80075848(nw4r::g3d::Camera* pSelf);
void fn_800758C8(nw4r::g3d::Camera* pSelf);
void fn_80075940(nw4r::g3d::Camera* pSelf);
void sin_cos_deg(f32* pOutSin, f32* pOutCos, f32 angle);
void fn_80075DD8(nw4r::g3d::Camera* pSelf);
f32 (*fn_80075E98(f32 (*pMtx)[4]))[4];

/* ------------------------------------------------------------------------------------------------ */
/* g3d_camera.cpp                                                                                    */
/* ------------------------------------------------------------------------------------------------ */

/* The `ResCommon<CameraData>::ref()`-style accessor (g3d_rescommon_ac.h:140). */
CameraData* fn_800748E4(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 140, "NW4R:Pointer must not be NULL (mpData)");
    }
    return pSelf->mpData;
}

/* The same accessor's second instantiation (g3d_rescommon_ac.h:143). */
CameraData* fn_80074A54(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 143, "NW4R:Pointer must not be NULL (mpData)");
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
    CameraData* pData = fn_800748E4(pSelf);
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
    pData->mProjB = fn_8004029C();
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

void fn_800749C8(nw4r::g3d::Camera* pSelf, nw4r::math::VEC3* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 195, "NW4R:Failed assertion IsValid()");
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = fn_80074A54(pSelf);
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
        CameraData* pData = fn_80074A54(pSelf);
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

/* True when the two three-float vectors differ. */
s32 fn_80074D38(const f32* pA, const f32* pB) {
    s32 result = 0;
    if (pA[0] != pB[0] || pA[1] != pB[1] || pA[2] != pB[2]) {
        result = 1;
    }
    return result;
}

void fn_80074D78(nw4r::g3d::Camera* pSelf, nw4r::g3d::Camera::PostureInfo* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 282, "NW4R:Failed assertion IsValid()");
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
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

void fn_80074F24(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 371, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_800748E4(pSelf);
        pData->mProjType = 1;
        pData->mUnkBC = a;
        pData->mUnkC0 = b;
        pData->mUnkC4 = c;
        pData->mUnkC8 = d;
        pData->mProjC = e;
        pData->mProjD = f;
        pData->mFlags &= ~0xF0;
        pData->mFlags |= 0x40;
    }
}

void fn_80074FEC(nw4r::g3d::Camera* pSelf, u32 x, u32 y, u32 w, u32 h) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 430, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_800748E4(pSelf);
        pData->mScissorX = x;
        pData->mScissorY = y;
        pData->mScissorW = w;
        pData->mScissorH = h;
    }
}

void fn_8007507C(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 463, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_800748E4(pSelf);
        pData->mViewportX = a;
        pData->mViewportY = b;
        pData->mViewportW = c;
        pData->mViewportH = d;
        fn_80074FEC(pSelf, (u32)d, (u32)c, (u32)b, (u32)a);
    }
}

void fn_80075170(nw4r::g3d::Camera* pSelf, f32* p1, f32* p2, f32* p3, f32* p4, f32* p5, f32* p6) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 521, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(pSelf)) {
        CameraData* pData = fn_80074A54(pSelf);
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

void fn_80075258(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec) {
    if (pOut == NULL) {
        return;
    }
    f32 viewMtx[12];
    f32 projMtx[16];
    ProjParams params;
    f32 frustum[6];
    MTX34_ctor((nw4r::math::MTX34*)viewMtx);
    MTX44_ctor((nw4r::math::MTX44*)projMtx);
    pSelf->GetCameraMtx((nw4r::math::MTX34*)viewMtx);
    camera_get_projection_mtx(pSelf, projMtx);
    CameraData* pData = fn_80074A54(pSelf);
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
    fn_80075170(pSelf, &frustum[0], &frustum[1], &frustum[2], &frustum[3], &frustum[4], &frustum[5]);
    mtx34_get_ptr(viewMtx);
    GXProject(viewMtx, &params.mProjType, frustum, pOut, pOut + 4, pOut + 8, pVec->x, pVec->y,
              pVec->z);
}

/* 0x80075390 (0x4): constructs a 4x4 matrix (nothing to initialise). */
void MTX44_ctor(nw4r::math::MTX44* pMtx) {
}

void nw4r::g3d::Camera::GetCameraMtx(math::MTX34* pMtx) const {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 629, "NW4R:Failed assertion IsValid()");
    }
    if (pMtx == NULL || !fn_80067EE8(this)) {
        return;
    }
    CameraData* pData = fn_80074A54((nw4r::g3d::Camera*)this);
    if ((pData->mFlags & 8) == 0) {
        fn_80075940((nw4r::g3d::Camera*)this);
    }
    mtx34_copy_ps((Mtx34*)pMtx, (const Mtx34*)pData);
}

void camera_get_projection_mtx(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 647, "NW4R:Failed assertion IsValid()");
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 0x80) == 0) {
        fn_80075DD8(pSelf);
    }
    nw4r::math::MTX44Copy((nw4r::math::MTX44*)pOut, (const nw4r::math::MTX44*)&pData->mProjMtx);
}

void camera_get_projection_tex_mtx(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 665, "NW4R:Failed assertion IsValid()");
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    u32 flags = pData->mFlags;
    if ((flags & 0x40) != 0) {
        mtx34_get_ptr(pOut);
        fn_804C6810(pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8, pData->mUnkCC,
                    -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
        return;
    }
    if ((flags & 0x10) != 0) {
        mtx34_get_ptr(pOut);
        fn_804C6660(pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8, pData->mProjC,
                    pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
        return;
    }
    mtx34_get_ptr(pOut);
    fn_804C6710(pData->mProjA, pData->mProjB, pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4,
                pData->mUnkD8);
}

void camera_get_environment_tex_mtx(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 713, "NW4R:Failed assertion IsValid()");
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    mtx34_identity(pOut);
    f32* pMtx = (f32*)pOut;
    pMtx[0] = pData->mUnkCC;
    pMtx[3] = pData->mUnkD4;
    pMtx[5] = -pData->mUnkD0;
    pMtx[7] = pData->mUnkD8;
    pMtx[10] = 0.0f;
    pMtx[11] = 1.0f;
}

#pragma peephole on
void fn_800756DC(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 736, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if (g3d_state_get_render_mode()->mUnk18 != 0) {
        GXSetViewportJitter((pData->mFlags & 0x100) != 0, pData->mViewportX, pData->mViewportY,
                    pData->mViewportW, pData->mViewportH, pData->mViewportNear, pData->mViewportFar);
        return;
    }
    GXSetViewport(pData->mViewportX, pData->mViewportY, pData->mViewportW, pData->mViewportH,
                  pData->mViewportNear, pData->mViewportFar);
}
#pragma peephole off

void fn_800757A8(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 773, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 0x80) == 0) {
        fn_80075DD8(pSelf);
    }
    fn_80075844(&pData->mProjMtx);
    GXSetProjection(&pData->mProjMtx[0][0], pData->mProjType);
}

void fn_80075844(void* pMtx) {
}

void fn_80075848(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 789, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    GXSetScissor(pData->mScissorX, pData->mScissorY, pData->mScissorW, pData->mScissorH);
}

void fn_800758C8(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic("g3d_camera.cpp", 806, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    GXSetScissorBoxOffset(pData->mScissorOffsetX, pData->mScissorOffsetY);
}


/* Rebuild the camera's view matrix from the posture the flags word selects: the +1 branch uses the
 * three-node path, +4 the position/target/up path and +2 the three-angle (Euler) path. */
void fn_80075940(nw4r::g3d::Camera* pSelf) {
    CameraData* pData = fn_80074A54(pSelf);
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
        s32 up = vec3_to_vec(&pData->mUpX);
        s32 target = vec3_to_vec(&pData->mTargetX);
        s32 pos = vec3_to_vec(&pData->mPosX);
        mtx34_get_ptr(pData);
        fn_804C64E0(pos, target, up);
    } else if ((flags & 4) != 0) {
        setVec3(&diff, pData->mPosX - pData->mUpX, pData->mPosY - pData->mUpY,
                    pData->mPosZ - pData->mUpZ);
        if (0.0f == diff.x && 0.0f == diff.z) {
            pData->mViewMtx[0][0] = 1.0f;
            pData->mViewMtx[0][1] = 0.0f;
            pData->mViewMtx[0][2] = 0.0f;
            pData->mViewMtx[0][3] = -pData->mPosX;
            pData->mViewMtx[1][0] = 0.0f;
            pData->mViewMtx[1][1] = 0.0f;
            pData->mViewMtx[2][0] = 0.0f;
            pData->mViewMtx[2][2] = 0.0f;
            if (diff.y <= 0.0f) {
                pData->mViewMtx[1][2] = 1.0f;
                pData->mViewMtx[1][3] = -pData->mPosZ;
                pData->mViewMtx[2][1] = -1.0f;
                pData->mViewMtx[2][3] = pData->mPosY;
            } else {
                pData->mViewMtx[1][2] = -1.0f;
                pData->mViewMtx[1][3] = pData->mPosZ;
                pData->mViewMtx[2][1] = 1.0f;
                pData->mViewMtx[2][3] = -pData->mPosY;
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
            pData->mViewMtx[0][3] = -vec3_dot(&pData->mPosX, &rowA);
            pData->mViewMtx[1][0] = rowB.x;
            pData->mViewMtx[1][1] = rowB.y;
            pData->mViewMtx[1][2] = rowB.z;
            pData->mViewMtx[1][3] = -vec3_dot(&pData->mPosX, &rowB);
            pData->mViewMtx[2][0] = diff.x;
            pData->mViewMtx[2][1] = diff.y;
            pData->mViewMtx[2][2] = diff.z;
            pData->mViewMtx[2][3] = -vec3_dot(&pData->mPosX, &diff);
        }
    } else {
        if ((flags & 2) == 0) {
            nw4r::db::Panic("g3d_camera.cpp", 926, "Unknown camera type(%x)");
        }
        sin_cos_deg(&sinX, &cosX, pData->mUnk98);
        sin_cos_deg(&sinY, &cosY, pData->mUnk9C);
        sin_cos_deg(&sinZ, &cosZ, pData->mUnkA0);
        VEC3_ctor(&row0);
        VEC3_ctor(&row1);
        VEC3_ctor(&row2);
        row0.x = sinZ * (sinX * sinY) + cosX * cosZ;
        row0.y = cosY * sinZ;
        row0.z = sinZ * (sinX * cosX) - sinY * cosZ;
        row1.x = cosZ * (sinX * sinY) - cosX * sinZ;
        row1.y = cosY * cosZ;
        row1.z = cosZ * (sinX * cosX) + sinY * sinZ;
        row2.x = cosY * sinY;
        row2.y = -sinX;
        row2.z = cosY * cosX;
        pData->mViewMtx[0][0] = row0.x;
        pData->mViewMtx[0][1] = row0.y;
        pData->mViewMtx[0][2] = row0.z;
        pData->mViewMtx[0][3] = -vec3_dot(&pData->mPosX, &row0);
        pData->mViewMtx[1][0] = row1.x;
        pData->mViewMtx[1][1] = row1.y;
        pData->mViewMtx[1][2] = row1.z;
        pData->mViewMtx[1][3] = -vec3_dot(&pData->mPosX, &row1);
        pData->mViewMtx[2][0] = row2.x;
        pData->mViewMtx[2][1] = row2.y;
        pData->mViewMtx[2][2] = row2.z;
        pData->mViewMtx[2][3] = -vec3_dot(&pData->mPosX, &row2);
    }
    pData->mFlags |= 8;
}


} /* extern "C" */

void nw4r::g3d::Camera::SetPosition(const math::VEC3& rPos) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 180, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = fn_800748E4(this);
        copyVec3((nw4r::math::VEC3*)&pData->mPosX, (const nw4r::math::VEC3*)&rPos);
        pData->mFlags &= ~8;
    }
}

void nw4r::g3d::Camera::SetPosture(const PostureInfo& rInfo) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 226, "NW4R:Failed assertion IsValid()");
    }
    if (!fn_80067EE8(this)) {
        return;
    }
    CameraData* pData = fn_800748E4(this);
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

void nw4r::g3d::Camera::SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic("g3d_camera.cpp", 329, "NW4R:Failed assertion IsValid()");
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = fn_800748E4(this);
        pData->mProjType = 0;
        pData->mProjA = fovy;
        pData->mProjB = aspect;
        pData->mProjC = near;
        pData->mProjD = far;
        pData->mFlags &= ~0xF0;
        pData->mFlags |= 0x20;
    }
}

extern "C" {

/* 0x80075DCC (0xC): the sine and cosine of `angle` degrees (nw4r's SinCosDeg: the 256-step table index is
 * `angle * 256 / 360`); the pointer pair passes through to math_sincos_idx untouched. */
void sin_cos_deg(f32* pOutSin, f32* pOutCos, f32 angle) {
    math_sincos_idx(pOutSin, pOutCos, lbl_80795DFC * angle);
}

/* 0x80075DD8 (0xC0): rebuilds the projection matrix from the camera's orthographic, frustum or perspective
 * parameters and marks it current. */
void fn_80075DD8(nw4r::g3d::Camera* pSelf) {
    CameraData* pData = fn_80074A54(pSelf);

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
