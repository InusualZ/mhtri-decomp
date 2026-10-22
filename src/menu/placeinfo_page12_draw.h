/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_placeinfo` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_PLACEINFO_PAGE12_DRAW_H
#define MHTRI_MENU_PLACEINFO_PAGE12_DRAW_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043B790 - menu page 12's draw (menu draw task, kind 12) */
void placeinfo_page12_draw(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACEINFO_PAGE12_DRAW_H */
