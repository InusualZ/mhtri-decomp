/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_EQUIP_DETAIL_SCREEN_DRAW_H
#define MHTRI_MENU_EQUIP_DETAIL_SCREEN_DRAW_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8030BD98 - menu page 8's draw (menu draw task, kind 8) */
void equip_detail_screen_draw(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EQUIP_DETAIL_SCREEN_DRAW_H */
