/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_item_page` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_ITEM_PAGE_DRAW_H
#define MHTRI_MENU_ITEM_PAGE_DRAW_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80349FF0 - draws the item page (menu page 3) */
void item_page_draw(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_ITEM_PAGE_DRAW_H */
