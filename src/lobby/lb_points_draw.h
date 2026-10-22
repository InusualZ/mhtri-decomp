/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s `lb_points_draw`, for a consumer that includes
 * `lobby/lb_list_init.h` (whose panel helpers `lobby/lb_panel_msg_draw.h` re-declares with other types).  C linkage
 * (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_POINTS_DRAW_H
#define MHTRI_LOBBY_LB_POINTS_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80215C04 - draws the resource-point total at layout `panel`. */
void lb_points_draw(u16 panel);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_POINTS_DRAW_H */
