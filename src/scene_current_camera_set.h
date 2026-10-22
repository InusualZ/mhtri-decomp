/* Leaf header (docs/plan.md 6.5 rule 2): `scene_current_camera_get`'s `scene_current_camera_set`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_SCENE_CURRENT_CAMERA_SET_H
#define MHTRI_SCENE_CURRENT_CAMERA_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800473F4 (scene_current_camera_get): makes the scene root's camera `index` the current one. */
void scene_current_camera_set(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCENE_CURRENT_CAMERA_SET_H */
