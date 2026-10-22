/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_plsearch.cpp`'s `lb_plsearch_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_PLSEARCH_OPEN_H
#define MHTRI_MENU_LB_PLSEARCH_OPEN_H

#include "types.h"

struct LbPlSearch;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044F530 - opens the player search for `target` (hud_key_lookup, menu_friend_list_init) */
void lb_plsearch_open(struct LbPlSearch* target);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_PLSEARCH_OPEN_H */
