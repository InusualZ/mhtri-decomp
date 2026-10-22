/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_plsearch` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_PLSEARCH_PAGE22_OPEN_H
#define MHTRI_MENU_PLSEARCH_PAGE22_OPEN_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044D050 - menu pages 22-24's opener (sub-list kinds 22-24) */
void plsearch_page22_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLSEARCH_PAGE22_OPEN_H */
