/* lobby/lb_ship_open.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_equip_page.cpp`'s `lb_ship_open`, for
 *   `lobby/fn_802FA9A0.cpp`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SHIP_OPEN_H
#define MHTRI_LOBBY_LB_SHIP_OPEN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80220038: opens the trade ship (its progress entry, the first two goods tables and one ship). */
void lb_ship_open(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SHIP_OPEN_H */
