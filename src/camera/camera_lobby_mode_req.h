/* Leaf header (docs/plan.md 6.5 rule 2): `camera/camera_main.cpp`'s `camera_lobby_mode_req`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_CAMERA_CAMERA_LOBBY_MODE_REQ_H
#define MHTRI_CAMERA_CAMERA_LOBBY_MODE_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BC7BC - requests camera mode 17 */
void camera_lobby_mode_req(s8 step);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_LOBBY_MODE_REQ_H */
