/*
 * Declarations owned by `g3d/g3d_camera.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.  Keep it minimal.
 *
 * `fn_80075258` projects a world position through the camera into screen space: `pSelf` is the camera
 * handle `fn_80082BCC` returns (only ever a pointer here), `pOut` the three-float result and `pVec`
 * the world position.  The owner's own spelling uses `nw4r::g3d::Camera*`; `extern "C"` keeps the map's
 * plain stem, which is what the call sites pair on.
 */
#ifndef MHTRI_G3D_G3D_CAMERA_H
#define MHTRI_G3D_G3D_CAMERA_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
struct Camera;
}  // namespace g3d
}  // namespace nw4r

extern "C" {
#endif

void fn_80075258(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec);

/* 0x80075390..0x80075620 - the matrix helpers `g3d/g3d_state.cpp` calls (rule 2, moved out of that
 * unit's local extern block on landing, 2026-09-25). */
void fn_80075390(void* pMtx);
void fn_80075394(void* pOut, const void* pIn);
void fn_80075440(void* pOut, const void* pIn);
void fn_800754EC(void* pOut, const void* pIn);
void fn_80075620(void* pOut, const void* pIn);

/* Added when `camera/fn_802B5C58.cpp` registered (rule 2): its `get_camera_pos` reads the camera's
 * position through this one. */
void fn_800749C8(const void* src, void* dst); /* 0x800749C8 - copies the camera's +0x74 vector out */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CAMERA_H */
