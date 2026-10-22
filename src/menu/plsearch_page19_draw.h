/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_plsearch` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_PLSEARCH_PAGE19_DRAW_H
#define MHTRI_MENU_PLSEARCH_PAGE19_DRAW_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044C794 - menu pages 19-21's draw (menu draw task, kinds 19-21) */
void plsearch_page19_draw(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLSEARCH_PAGE19_DRAW_H */
