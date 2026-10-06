/* Leaf header (docs/plan.md 6.5 rule 2): the lobby talk-menu opener of `lobby/fn_80219260.cpp` the lobby screens call
 * (the page's other entry points are `lobby/lb_talk_page_open.h`'s).  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_MENU_OPEN_H
#define MHTRI_LOBBY_LB_MENU_OPEN_H

#include "types.h"

struct _LB_NPC;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021CA64 - opens the talk menu of NPC `npc` (its script by the NPC's kind) (GUESS name). */
void lb_menu_open(struct _LB_NPC* npc);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_MENU_OPEN_H */
