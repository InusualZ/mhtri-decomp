/* g3d/camera_posture_get.h - leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_camera.cpp`'s camera posture read, for `menu/movie.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_G3D_CAMERA_POSTURE_GET_H
#define MHTRI_G3D_CAMERA_POSTURE_GET_H

#include "types.h"
#include "g3d/g3d_camera_types.h"

/* 0x80074D78 - copies `camera`'s position, target and up vectors into `posture`. */
extern "C" void camera_posture_get(nw4r::g3d::Camera* camera, nw4r::g3d::Camera::PostureInfo* posture);

#endif /* MHTRI_G3D_CAMERA_POSTURE_GET_H */
