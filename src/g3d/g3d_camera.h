/* g3d/g3d_camera.h - the declarations `g3d/g3d_camera.cpp` owns: fn_80075258 projects the world position `pVec`
 *   through the camera handle `pSelf` (what fn_80082BCC returns) into the screen-space `pOut`; `extern "C"` keeps the
 *   map's stem. */
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

/* 0x80075390..0x80075620 - the matrix helpers (0x80075394 is the member `Camera::GetCameraMtx`) `g3d/g3d_state.cpp` calls (rule 2, moved out of that
 * unit's local extern block on landing). */
void MTX44_ctor(nw4r::math::MTX44* pMtx);
/* 0x800746DC (0x7C): resets the camera record the handle names to its defaults. */
void camera_init(nw4r::g3d::Camera* pSelf);
void fn_80075440(void* pOut, const void* pIn);
void fn_800754EC(void* pOut, const void* pIn);
void fn_80075620(void* pOut, const void* pIn);

/* The camera-position copy `camera/fn_802B5C58.cpp`'s `get_camera_pos` reads through. */
void fn_800749C8(const void* src, void* dst); /* 0x800749C8 - copies the camera's +0x74 vector out */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CAMERA_H */
