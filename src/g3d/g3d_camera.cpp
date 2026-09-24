/*
 * nw4r g3d: g3d_camera.cpp - `Camera`'s posture/projection/viewport/scissor methods and the
 * `ResCommon<CameraData>` accessors, `.text` 0x800746DC-0x80075DCC (27 functions, including the
 * already-demangled `Camera::SetPosition`/`SetPosture`/`SetPerspective`).
 *
 * Re-cut from `auto/80073398_fn_80073398.cpp` (docs/plan.md 12 item 5).  The `g3d_camera.cpp` string
 * `lbl_8058E430` is referenced from `fn_800746DC` onward and `g3d_rescommon_ac.h` from
 * `fn_800748E4`/`fn_80074A54`; the data fragment is 0x8058E430-0x8058E570.  The six accessors
 * `fn_80074620`..`fn_800746D4` (0x80074620-0x800746DC, zero data references) are assigned to
 * `g3d/g3d_calcworld.cpp` per the report's candidate cut - unpinned, measure to settle.
 *
 * rule 7 deferred: the free functions keep the map's `fn_XXXXXXXX` names (docs/plan.md 6.5 rule 7);
 * the three `Camera` members are named because the map already carries their mangling.
 *
 * Shared declarations: `nw4r::g3d::Camera`/`CameraData` and `nw4r::math::VEC3` are local/shared per
 * rule 1 (`VEC3` comes from `nw4r/math.h`).  `#pragma peephole off` is scoped to `fn_80075940`
 * (playbook 32) and `#pragma fp_contract off` is file-scoped.  Registered `Object(NonMatching, ...)`
 * in lib g3d.
 */


#include "types.h"
#include "nw4r/math.h"

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
extern "C" ScissorSize* fn_8004028C(void);
extern "C" f32 fn_8004029C(void);
extern "C" void fn_80041E40(void* pDst, const void* pSrc);
extern "C" void fn_80041E8C(void* pOut, f32 x, f32 y, f32 z);
extern "C" void fn_80043EA8(void* pOut);
extern "C" void fn_800504D4(void* pOut);
extern "C" void* fn_80050508(void* pMtx);
extern "C" void fn_8005050C(void* pOut);
extern "C" void fn_80050850(void* pOut, const void* pIn);
extern "C" s32 fn_800508A8(const void* pIn);
extern "C" void fn_80051820(void* pOut, const void* pA, const void* pB);
extern "C" f32 fn_80052214(const void* pA, const void* pB);
extern "C" s32 fn_80067EE8(const void* p);
extern "C" void fn_8007100C(void* pDst, const void* pSrc);
extern "C" void fn_80075DCC(f32* pOutSin, f32* pOutCos, f32 angle);
extern "C" void fn_80075DD8(void* p);
extern "C" RenderModeObj* fn_80088584(void);
extern "C" void fn_804BA230(const f32* pViewMtx, const f32* pParams, const f32* pFrustum, void* pA,
                            void* pB, void* pC, f32 x, f32 y, f32 z);
extern "C" void fn_804BA7A0(s32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g);
extern "C" void fn_804C64E0(s32 a, s32 b, s32 c);
extern "C" void fn_804C6660(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h, f32 i);
extern "C" void fn_804C6710(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
extern "C" void fn_804C6810(f32 a, f32 b, f32 c, f32 d, f32 e, f32 f, f32 g, f32 h);
extern "C" void fn_8050168C(void* pOut, const void* pIn);

/* The panic file/format strings the target references as map symbols. They are extern here rather
 * than literals: MWCC's `-str reuse` would pool a literal into one blob and address it through a
 * shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058E430[];
extern const char lbl_8058E440[];
extern const char lbl_8058E468[];
extern const char lbl_8058E488[];
extern const char lbl_8058E4E0[];
extern const char lbl_8058E4F8[];
extern const char lbl_8058E520[];
extern const char lbl_8058E534[];
extern const char lbl_8058E55C[];

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The render-mode object `fn_80088584` returns (`GXRenderModeObj`-shaped). */
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

/* The seven-float projection parameter block `fn_804BA230` reads at its second argument. */
struct ProjParams {
    /* +0x00 */ f32 mProjType;
    /* +0x04 */ f32 mUnk04;
    /* +0x08 */ f32 mUnk08;
    /* +0x0C */ f32 mUnk0C;
    /* +0x10 */ f32 mUnk10;
    /* +0x14 */ f32 mUnk14;
    /* +0x18 */ f32 mUnk18;
}; /* size: 0x1C */

/* The camera payload `Camera::mpData` points at (`g3d_camera.cpp`'s `CameraData`). The 0x70 flag word
 * records which posture/projection branch is current. */
struct CameraData {
    /* +0x00 */ f32 mViewMtx[3][4];
    /* +0x30 */ f32 mProjMtx[3][4];
    /* +0x60 */ u32 mUnk60[4];
    /* +0x70 */ u32 mFlags;
    /* +0x74 */ f32 mPosX;
    /* +0x78 */ f32 mPosY;
    /* +0x7C */ f32 mPosZ;
    /* +0x80 */ f32 mTargetX;
    /* +0x84 */ f32 mTargetY;
    /* +0x88 */ f32 mTargetZ;
    /* +0x8C */ f32 mUpX;
    /* +0x90 */ f32 mUpY;
    /* +0x94 */ f32 mUpZ;
    /* +0x98 */ f32 mUnk98;
    /* +0x9C */ f32 mUnk9C;
    /* +0xA0 */ f32 mUnkA0;
    /* +0xA4 */ f32 mUnkA4;
    /* +0xA8 */ s32 mProjType;
    /* +0xAC */ f32 mProjA;
    /* +0xB0 */ f32 mProjB;
    /* +0xB4 */ f32 mProjC;
    /* +0xB8 */ f32 mProjD;
    /* +0xBC */ f32 mUnkBC;
    /* +0xC0 */ f32 mUnkC0;
    /* +0xC4 */ f32 mUnkC4;
    /* +0xC8 */ f32 mUnkC8;
    /* +0xCC */ f32 mUnkCC;
    /* +0xD0 */ f32 mUnkD0;
    /* +0xD4 */ f32 mUnkD4;
    /* +0xD8 */ f32 mUnkD8;
    /* +0xDC */ f32 mViewportX;
    /* +0xE0 */ f32 mViewportY;
    /* +0xE4 */ f32 mViewportW;
    /* +0xE8 */ f32 mViewportH;
    /* +0xEC */ f32 mViewportNear;
    /* +0xF0 */ f32 mViewportFar;
    /* +0xF4 */ s32 mScissorX;
    /* +0xF8 */ s32 mScissorY;
    /* +0xFC */ s32 mScissorW;
    /* +0x100 */ s32 mScissorH;
    /* +0x104 */ s32 mScissorOffsetX;
    /* +0x108 */ s32 mScissorOffsetY;
}; /* size: 0x10C */

/* `nw4r::g3d::Camera` itself: a `ResCommon<CameraData>` handle, so the payload is the first word. The
 * three mangled map symbols require the real class, which is why this one is a class and not a struct. */
namespace nw4r {
namespace g3d {

class Camera {
public:
    struct PostureInfo {
        /* +0x00 */ s32 mType;
        /* +0x04 */ f32 mPosX;
        /* +0x08 */ f32 mPosY;
        /* +0x0C */ f32 mPosZ;
        /* +0x10 */ f32 mTargetX;
        /* +0x14 */ f32 mTargetY;
        /* +0x18 */ f32 mTargetZ;
        /* +0x1C */ f32 mUpX;
        /* +0x20 */ f32 mUpY;
        /* +0x24 */ f32 mUpZ;
        /* +0x28 */ f32 mUnk28;
    }; /* size: 0x2C */

    void SetPosition(const math::VEC3& rPos);
    void SetPosture(const PostureInfo& rInfo);
    void SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far);

    /* +0x0 */ CameraData* mpData;
}; /* size: 0x4 */

}  // namespace g3d
}  // namespace nw4r

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

CameraData* fn_800748E4(nw4r::g3d::Camera* pSelf);
CameraData* fn_80074A54(nw4r::g3d::Camera* pSelf);
void fn_800746DC(nw4r::g3d::Camera* pSelf);
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
void fn_80075390(void* pOut);
void fn_80075394(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_80075440(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_800754EC(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_80075620(nw4r::g3d::Camera* pSelf, void* pOut);
void fn_800756DC(nw4r::g3d::Camera* pSelf);
void fn_800757A8(nw4r::g3d::Camera* pSelf);
void fn_80075844(void* pMtx);
void fn_80075848(nw4r::g3d::Camera* pSelf);
void fn_800758C8(nw4r::g3d::Camera* pSelf);
void fn_80075940(nw4r::g3d::Camera* pSelf);

/* ------------------------------------------------------------------------------------------------ */
/* g3d_camera.cpp                                                                                    */
/* ------------------------------------------------------------------------------------------------ */

/* The `ResCommon<CameraData>::ref()`-style accessor (g3d_rescommon_ac.h:140). */
CameraData* fn_800748E4(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic(lbl_8058E520, 140, lbl_8058E4F8);
    }
    return pSelf->mpData;
}

/* The same accessor's second instantiation (g3d_rescommon_ac.h:143). */
CameraData* fn_80074A54(nw4r::g3d::Camera* pSelf) {
    if (pSelf->mpData == NULL) {
        nw4r::db::Panic(lbl_8058E55C, 143, lbl_8058E534);
    }
    return pSelf->mpData;
}

/* Reset every camera field from the current render mode. */
void fn_800746DC(nw4r::g3d::Camera* pSelf) {
    RenderModeObj* pMode = (RenderModeObj*)fn_80088584();
    if (pMode == NULL) {
        nw4r::db::Panic(lbl_8058E430, 50, lbl_8058E440);
    }
    u16 width = pMode->mUnk04;
    fn_80074758(pSelf, width, pMode->mUnk06, width, pMode->mUnk08, pMode->mUnk0E, pMode->mUnk10);
}

void fn_80074758(nw4r::g3d::Camera* pSelf, u16 a1, u16 a2, u16 a3, u16 a4, u16 a5, u16 a6) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 62, lbl_8058E468);
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
        nw4r::db::Panic(lbl_8058E430, 195, lbl_8058E468);
    }
    if (pOut != NULL && fn_80067EE8(pSelf)) {
        CameraData* pData = fn_80074A54(pSelf);
        fn_80041E40(pOut, &pData->mPosX);
    }
}

void fn_80074AA8(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pZ) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 206, lbl_8058E468);
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
        nw4r::db::Panic(lbl_8058E430, 282, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    u32 flags = pData->mFlags;
    if ((flags & 1) != 0) {
        pOut->mType = 0;
        fn_80041E40(&pOut->mPosX, &pData->mTargetX);
        fn_80041E40(&pOut->mTargetX, &pData->mUpX);
        return;
    }
    if ((flags & 2) != 0) {
        pOut->mType = 1;
        fn_80041E40(&pOut->mUpX, &pData->mUnk98);
        return;
    }
    pOut->mType = 2;
    fn_80041E40(&pOut->mTargetX, &pData->mUpX);
    pOut->mUnk28 = pData->mUnkA4;
}

void fn_80074F24(nw4r::g3d::Camera* pSelf, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 371, lbl_8058E468);
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
        nw4r::db::Panic(lbl_8058E430, 430, lbl_8058E468);
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
        nw4r::db::Panic(lbl_8058E430, 463, lbl_8058E468);
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
        nw4r::db::Panic(lbl_8058E430, 521, lbl_8058E468);
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
    fn_8005050C(viewMtx);
    fn_80075390(projMtx);
    fn_80075394(pSelf, viewMtx);
    fn_80075440(pSelf, projMtx);
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
    fn_80050508(viewMtx);
    fn_804BA230(viewMtx, &params.mProjType, frustum, pOut, pOut + 4, pOut + 8, pVec->x, pVec->y,
                pVec->z);
}

void fn_80075390(void* pOut) {
}

void fn_80075394(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 629, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 8) == 0) {
        fn_80075940(pSelf);
    }
    fn_8007100C(pOut, pData);
}

void fn_80075440(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 647, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if ((pData->mFlags & 0x80) == 0) {
        fn_80075DD8(pSelf);
    }
    fn_8050168C(pOut, &pData->mProjMtx);
}

void fn_800754EC(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 665, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    u32 flags = pData->mFlags;
    if ((flags & 0x40) != 0) {
        fn_80050508(pOut);
        fn_804C6810(pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8, pData->mUnkCC,
                    -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
        return;
    }
    if ((flags & 0x10) != 0) {
        fn_80050508(pOut);
        fn_804C6660(pData->mUnkBC, pData->mUnkC0, pData->mUnkC4, pData->mUnkC8, pData->mProjC,
                    pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4, pData->mUnkD8);
        return;
    }
    fn_80050508(pOut);
    fn_804C6710(pData->mProjA, pData->mProjB, pData->mUnkCC, -pData->mUnkD0, pData->mUnkD4,
                pData->mUnkD8);
}

void fn_80075620(nw4r::g3d::Camera* pSelf, void* pOut) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 713, lbl_8058E468);
    }
    if (pOut == NULL || !fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    fn_800504D4(pOut);
    f32* pMtx = (f32*)pOut;
    pMtx[0] = pData->mUnkCC;
    pMtx[3] = pData->mUnkD4;
    pMtx[5] = -pData->mUnkD0;
    pMtx[7] = pData->mUnkD8;
    pMtx[10] = 0.0f;
    pMtx[11] = 1.0f;
}

void fn_800756DC(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 736, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    if (fn_80088584()->mUnk18 != 0) {
        fn_804BA7A0((pData->mFlags & 0x100) != 0, pData->mViewportX, pData->mViewportY,
                    pData->mViewportW, pData->mViewportH, pData->mViewportNear, pData->mViewportFar);
        return;
    }
    GXSetViewport(pData->mViewportX, pData->mViewportY, pData->mViewportW, pData->mViewportH,
                  pData->mViewportNear, pData->mViewportFar);
}

void fn_800757A8(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 773, lbl_8058E468);
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
        nw4r::db::Panic(lbl_8058E430, 789, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    GXSetScissor(pData->mScissorX, pData->mScissorY, pData->mScissorW, pData->mScissorH);
}

void fn_800758C8(nw4r::g3d::Camera* pSelf) {
    if (!fn_80067EE8(pSelf)) {
        nw4r::db::Panic(lbl_8058E430, 806, lbl_8058E468);
    }
    if (!fn_80067EE8(pSelf)) {
        return;
    }
    CameraData* pData = fn_80074A54(pSelf);
    GXSetScissorBoxOffset(pData->mScissorOffsetX, pData->mScissorOffsetY);
}

#pragma peephole off

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
            nw4r::db::Panic(lbl_8058E430, 832, lbl_8058E488, pData->mPosX, pData->mPosY,
                            pData->mPosZ);
        }
        s32 up = fn_800508A8(&pData->mUpX);
        s32 target = fn_800508A8(&pData->mTargetX);
        s32 pos = fn_800508A8(&pData->mPosX);
        fn_80050508(pData);
        fn_804C64E0(pos, target, up);
    } else if ((flags & 4) != 0) {
        fn_80041E8C(&diff, pData->mPosX - pData->mUpX, pData->mPosY - pData->mUpY,
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
            fn_80041E8C(&axisA, diff.z, 0.0f, -diff.x);
            fn_80043EA8(&axisB);
            fn_80050850(&diff, &diff);
            fn_80050850(&axisA, &axisA);
            fn_80051820(&axisB, &diff, &axisA);
            fn_80075DCC(&rotSin, &rotCos, pData->mUnkA4);
            fn_80043EA8(&rowA);
            fn_80043EA8(&rowB);
            rowA.x = rotSin * axisB.x + rotCos * axisA.x;
            rowA.y = rotSin * axisB.y;
            rowA.z = rotSin * axisB.z + rotCos * axisA.z;
            rowB.x = rotCos * axisB.x - rotSin * axisA.x;
            rowB.y = rotCos * axisB.y;
            rowB.z = rotCos * axisB.z - rotSin * axisA.z;
            pData->mViewMtx[0][0] = rowA.x;
            pData->mViewMtx[0][1] = rowA.y;
            pData->mViewMtx[0][2] = rowA.z;
            pData->mViewMtx[0][3] = -fn_80052214(&pData->mPosX, &rowA);
            pData->mViewMtx[1][0] = rowB.x;
            pData->mViewMtx[1][1] = rowB.y;
            pData->mViewMtx[1][2] = rowB.z;
            pData->mViewMtx[1][3] = -fn_80052214(&pData->mPosX, &rowB);
            pData->mViewMtx[2][0] = diff.x;
            pData->mViewMtx[2][1] = diff.y;
            pData->mViewMtx[2][2] = diff.z;
            pData->mViewMtx[2][3] = -fn_80052214(&pData->mPosX, &diff);
        }
    } else {
        if ((flags & 2) == 0) {
            nw4r::db::Panic(lbl_8058E430, 926, lbl_8058E4E0);
        }
        fn_80075DCC(&sinX, &cosX, pData->mUnk98);
        fn_80075DCC(&sinY, &cosY, pData->mUnk9C);
        fn_80075DCC(&sinZ, &cosZ, pData->mUnkA0);
        fn_80043EA8(&row0);
        fn_80043EA8(&row1);
        fn_80043EA8(&row2);
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
        pData->mViewMtx[0][3] = -fn_80052214(&pData->mPosX, &row0);
        pData->mViewMtx[1][0] = row1.x;
        pData->mViewMtx[1][1] = row1.y;
        pData->mViewMtx[1][2] = row1.z;
        pData->mViewMtx[1][3] = -fn_80052214(&pData->mPosX, &row1);
        pData->mViewMtx[2][0] = row2.x;
        pData->mViewMtx[2][1] = row2.y;
        pData->mViewMtx[2][2] = row2.z;
        pData->mViewMtx[2][3] = -fn_80052214(&pData->mPosX, &row2);
    }
    pData->mFlags |= 8;
}

#pragma peephole on

} /* extern "C" */

void nw4r::g3d::Camera::SetPosition(const math::VEC3& rPos) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic(lbl_8058E430, 180, lbl_8058E468);
    }
    if (fn_80067EE8(this)) {
        CameraData* pData = fn_800748E4(this);
        fn_80041E40(&pData->mPosX, &rPos);
        pData->mFlags &= ~8;
    }
}

void nw4r::g3d::Camera::SetPosture(const PostureInfo& rInfo) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic(lbl_8058E430, 226, lbl_8058E468);
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
            fn_80041E40(&pData->mTargetX, &rInfo.mPosX);
            fn_80041E40(&pData->mUpX, &rInfo.mTargetX);
            pData->mFlags &= ~8;
        }
        break;
    }
    case 1: {
        if ((pData->mFlags & 2) == 0 || fn_80074D38(&rInfo.mUpX, &pData->mUnk98) != 0) {
            pData->mFlags &= ~7;
            pData->mFlags |= 2;
            fn_80041E40(&pData->mUnk98, &rInfo.mUpX);
            pData->mFlags &= ~8;
        }
        break;
    }
    case 2: {
        if ((pData->mFlags & 4) == 0 || fn_80074D38(&rInfo.mTargetX, &pData->mUpX) != 0 ||
            rInfo.mUnk28 != pData->mUnkA4) {
            pData->mFlags &= ~7;
            pData->mFlags |= 4;
            fn_80041E40(&pData->mUpX, &rInfo.mTargetX);
            pData->mUnkA4 = rInfo.mUnk28;
            pData->mFlags &= ~8;
        }
        break;
    }
    }
}

void nw4r::g3d::Camera::SetPerspective(f32 fovy, f32 aspect, f32 near, f32 far) {
    if (!fn_80067EE8(this)) {
        nw4r::db::Panic(lbl_8058E430, 329, lbl_8058E468);
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
