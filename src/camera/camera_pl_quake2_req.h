/* Leaf header (docs/plan.md 6.5 rule 2): `camera/camera_main.cpp`'s `camera_pl_quake2_req`, which `Pl/fn_802840DC.cpp` calls. */
#ifndef MHTRI_CAMERA_CAMERA_PL_QUAKE2_REQ_H
#define MHTRI_CAMERA_CAMERA_PL_QUAKE2_REQ_H

#include "types.h"

struct _PLW;

extern "C" {

/* 0x802BE714 - requests the player's second quake slot. */
void camera_pl_quake2_req(struct _PLW* player, u8 kind);

}

#endif /* MHTRI_CAMERA_CAMERA_PL_QUAKE2_REQ_H */
