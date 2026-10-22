/* Leaf header (docs/plan.md 6.5 rule 2): the `ef/fn_8030681C` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_EF_MENU_STATUS_PAGE_OPEN_H
#define MHTRI_EF_MENU_STATUS_PAGE_OPEN_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80308EC0 - menu page 7's opener (list 1 row 1) */
void menu_status_page_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_MENU_STATUS_PAGE_OPEN_H */
