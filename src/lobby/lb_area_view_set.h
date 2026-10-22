/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80219260.cpp`'s `lb_area_view_set`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_AREA_VIEW_SET_H
#define MHTRI_LOBBY_LB_AREA_VIEW_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021D7C4 - sets the menu scratch's view for area `area` (get_option_pair, vec_to_mh_vec3) */
void lb_area_view_set(u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_AREA_VIEW_SET_H */
