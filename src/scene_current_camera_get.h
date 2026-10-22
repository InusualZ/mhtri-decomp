/* Leaf header (docs/plan.md 6.5 rule 2): `scene_current_camera_get`'s `scene_current_camera_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_SCENE_CURRENT_CAMERA_GET_H
#define MHTRI_SCENE_CURRENT_CAMERA_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80047398 (scene_current_camera_get): the scene root's current camera. */
void* scene_current_camera_get(void); /* untyped: opaque handle - the scene root's camera word */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCENE_CURRENT_CAMERA_GET_H */
