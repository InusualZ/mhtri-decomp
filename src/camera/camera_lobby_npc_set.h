/* `camera_lobby_npc_set`, defined by `camera/camera_main.cpp` (leaf header, rule 2). */
#ifndef MHTRI_CAMERA_CAMERA_LOBBY_NPC_SET_H
#define MHTRI_CAMERA_CAMERA_LOBBY_NPC_SET_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* Turns the lobby camera to NPC `npc`. */
void camera_lobby_npc_set(struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_LOBBY_NPC_SET_H */
