/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_802FA9A0.cpp`'s `lb_pig_dress_refresh`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_PIG_DRESS_REFRESH_H
#define MHTRI_LOBBY_LB_PIG_DRESS_REFRESH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802FBA14 - refreshes the pig's dress and the NPC dresses (lb_npc_dress_refresh_all) */
void lb_pig_dress_refresh(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PIG_DRESS_REFRESH_H */
