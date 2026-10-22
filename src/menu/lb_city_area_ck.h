/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_placeinfo.cpp`'s `lb_city_area_ck`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_MENU_LB_CITY_AREA_CK_H
#define MHTRI_MENU_LB_CITY_AREA_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043C738 - 1 for areas 5..7 of map 21 and area 1 of map 22 */
u32 lb_city_area_ck(u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_LB_CITY_AREA_CK_H */
