/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_menu_scratch.cpp`'s `lb_plsearch_target_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_PLSEARCH_TARGET_GET_H
#define MHTRI_LOBBY_LB_PLSEARCH_TARGET_GET_H

#include "types.h"

struct LbPlSearch;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021EC20 - returns the menu scratch's player-search record */
struct LbPlSearch* lb_plsearch_target_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PLSEARCH_TARGET_GET_H */
