/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_camera`'s `camera_set_viewport`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_G3D_CAMERA_SET_VIEWPORT_H
#define MHTRI_G3D_CAMERA_SET_VIEWPORT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r { namespace g3d { class Camera; } }
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8007507C (g3d/g3d_camera): sets the g3d camera's viewport. */
void camera_set_viewport(nw4r::g3d::Camera* cam, f32 x, f32 y, f32 w, f32 h);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_CAMERA_SET_VIEWPORT_H */
