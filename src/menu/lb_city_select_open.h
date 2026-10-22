/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_placeinfo.cpp`'s `lb_city_select_open`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_CITY_SELECT_OPEN_H
#define MHTRI_MENU_LB_CITY_SELECT_OPEN_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043733C - opens the city select (screen 28 for kind 11, else 33) */
void lb_city_select_open(struct _PLW* self, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_CITY_SELECT_OPEN_H */
