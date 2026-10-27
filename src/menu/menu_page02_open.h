/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_combo` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_PAGE02_OPEN_H
#define MHTRI_MENU_MENU_PAGE02_OPEN_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031B6D0 - menu page 2's opener (the top menu's list 0 row 2) */
void menu_page02_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_PAGE02_OPEN_H */
