/* camera/camera_angle_x_get.h - leaf header (docs/plan.md 6.5 rule 2): `camera/camera_main.cpp`'s
 *   `camera_angle_x_get`, `camera_angle_y_get` and `camera_underwater_ck`, for `lobby/fn_802FA9A0.cpp`.  C linkage
 *   (the owner defines them `extern "C"`); the signatures are the owner's definitions. */
#ifndef MHTRI_CAMERA_CAMERA_ANGLE_X_GET_H
#define MHTRI_CAMERA_CAMERA_ANGLE_X_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BDEF8 / 0x802BDF5C: the current camera's pitch and yaw angles. */
u32 camera_angle_x_get(void);
s32 camera_angle_y_get(void);
/* 0x802BE088: whether the camera is under the water surface. */
u32 camera_underwater_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_ANGLE_X_GET_H */
