/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/lb_npc.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_NPC_AREA_LIGHT_SET_H
#define MHTRI_LOBBY_LB_NPC_AREA_LIGHT_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801FF668 - selects the lobby light of area `area` and refreshes every NPC's dress. */
void lb_npc_area_light_set(u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_AREA_LIGHT_SET_H */
