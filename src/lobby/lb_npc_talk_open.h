/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_npc_talk_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_NPC_TALK_OPEN_H
#define MHTRI_LOBBY_LB_NPC_TALK_OPEN_H

#include "types.h"

struct _PLW;
struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021C938 - opens the talk with NPC `npc` for player `self` (lb_npc_talk_select) */
void lb_npc_talk_open(struct _PLW* self, struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_NPC_TALK_OPEN_H */
