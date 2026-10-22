/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s `lb_scene_npc_refresh_all`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_SCENE_NPC_REFRESH_ALL_H
#define MHTRI_LOBBY_LB_SCENE_NPC_REFRESH_ALL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021793C - tail-calls lb_scene_npc_refresh(1) */
void lb_scene_npc_refresh_all(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCENE_NPC_REFRESH_ALL_H */
