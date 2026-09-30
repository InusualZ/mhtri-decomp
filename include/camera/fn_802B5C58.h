/*
 * camera/fn_802B5C58.h - the C-linkage declarations `src/camera/fn_802B5C58.cpp` owns
 * (docs/plan.md 6.5 rule 2).  The unit's C++ entry points are in `camera/camera.h`.
 */
#ifndef MHTRI_CAMERA_FN_802B5C58_H
#define MHTRI_CAMERA_FN_802B5C58_H

#include "types.h"
#include "nw4r/math.h"
#include "g3d/g3d_camera_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BE1DC - moves `camera` to `pos` when `pos` is non-NULL (a tail call into `Camera::SetPosition`). */
void camera_position_set(nw4r::g3d::Camera* camera, const nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_FN_802B5C58_H */
