/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_camera`'s `camera_get_position`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_G3D_CAMERA_GET_POSITION_H
#define MHTRI_G3D_CAMERA_GET_POSITION_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800749C8 (g3d/g3d_camera): copies the g3d camera's position out. */
void camera_get_position(const void* src /* untyped: opaque handle - the g3d camera */, void* dst /* untyped: byte range - the 0xC-byte position */);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_CAMERA_GET_POSITION_H */
