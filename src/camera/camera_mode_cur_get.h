/* Leaf header (docs/plan.md 6.5 rule 2): `camera/camera_main.cpp`'s `camera_mode_cur_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_CAMERA_CAMERA_MODE_CUR_GET_H
#define MHTRI_CAMERA_CAMERA_MODE_CUR_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BBA94 - returns the camera's current mode byte (+0x460) */
u8 camera_mode_cur_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_MODE_CUR_GET_H */
