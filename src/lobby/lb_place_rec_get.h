/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_menu_scratch.cpp`'s `lb_place_rec_get`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_PLACE_REC_GET_H
#define MHTRI_LOBBY_LB_PLACE_REC_GET_H

#include "types.h"

struct LbPlaceRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021EC74 - returns the menu scratch's place record */
struct LbPlaceRec* lb_place_rec_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PLACE_REC_GET_H */
