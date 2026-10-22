/* Leaf header (docs/plan.md 6.5 rule 2): `pad_connect`'s `scene_camera_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_SCENE_CAMERA_GET_H
#define MHTRI_SCENE_CAMERA_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80047318 (pad_connect): the scene root's camera `index` (offset by the split-screen player's camera bank in split screen). */
s32 scene_camera_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCENE_CAMERA_GET_H */
