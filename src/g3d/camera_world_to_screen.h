/* Leaf header (docs/plan.md 6.5 rule 2): `g3d/g3d_camera.cpp`'s `camera_world_to_screen`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_G3D_CAMERA_WORLD_TO_SCREEN_H
#define MHTRI_G3D_CAMERA_WORLD_TO_SCREEN_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80075258 - projects world position `pos` to screen `screen` through the camera matrix, projection and viewport (GXProject) */
void camera_world_to_screen(u32* camera, VEC3* screen, VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_CAMERA_WORLD_TO_SCREEN_H */
