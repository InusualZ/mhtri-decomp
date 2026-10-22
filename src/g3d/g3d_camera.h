/* g3d/g3d_camera.h - the declarations `g3d/g3d_camera.cpp` owns: camera_world_to_screen projects the world position `pVec`
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

void camera_world_to_screen(nw4r::g3d::Camera* pSelf, u8* pOut, const nw4r::math::VEC3* pVec);

/* 0x80075DCC (0xC): the sine and cosine of `angle` degrees through the 256-step table. */
void sin_cos_deg(f32* pOutSin, f32* pOutCos, f32 angle);

/* 0x80075390..0x80075620 - the matrix helpers (0x80075394 is the member `Camera::GetCameraMtx`) `g3d/g3d_state.cpp` calls (rule 2, moved out of that
 * unit's local extern block on landing). */
void MTX44_ctor(nw4r::math::MTX44* pMtx);
/* 0x800746DC (0x7C): resets the camera record the handle names to its defaults. */
void camera_init(nw4r::g3d::Camera* pSelf);
/* 0x80075440 (0xAC): copies the camera's projection matrix (rebuilt first when stale) into `pOut`. */
void camera_get_projection_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX44* pOut);
/* 0x800748E4 / 0x80074A54 (0x54 each): the camera record, asserting a valid handle (ResCommon::ref and ref const). */
struct CameraData* camera_ref(nw4r::g3d::Camera* pSelf);
struct CameraData* camera_ref_const(const nw4r::g3d::Camera* pSelf);
/* 0x80075170 (0xE8): reads the viewport through the non-null pointers. */
void camera_get_viewport(nw4r::g3d::Camera* pSelf, f32* pX, f32* pY, f32* pWidth, f32* pHeight, f32* pNear,
                         f32* pFar);
/* 0x800756DC..0x800758C8: load the camera's viewport, projection, scissor and scissor offset into GX. */
void camera_gx_set_viewport(nw4r::g3d::Camera* pSelf);
void camera_gx_set_projection(nw4r::g3d::Camera* pSelf);
void camera_gx_set_scissor(nw4r::g3d::Camera* pSelf);
void camera_gx_set_scissor_box_offset(nw4r::g3d::Camera* pSelf);
/* 0x800754EC (0x134) / 0x80075620 (0xBC): the camera's texture projection and environment texture matrices. */
void camera_get_projection_tex_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX34* pOut);
void camera_get_environment_tex_mtx(const nw4r::g3d::Camera* pSelf, nw4r::math::MTX34* pOut);

/* The camera-position copy `camera/fn_802B5C58.cpp`'s `get_camera_pos` reads through. */
void fn_800749C8(const void* src, void* dst); /* 0x800749C8 - copies the camera's +0x74 vector out */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_CAMERA_H */
