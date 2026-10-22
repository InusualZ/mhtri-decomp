/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/fn_8031A6C0` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_PAGE02_DRAW_H
#define MHTRI_MENU_MENU_PAGE02_DRAW_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031BD7C - menu page 2's draw (menu draw task, kind 2) */
void menu_page02_draw(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_PAGE02_DRAW_H */
