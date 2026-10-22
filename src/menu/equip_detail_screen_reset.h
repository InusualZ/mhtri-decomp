/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_EQUIP_DETAIL_SCREEN_RESET_H
#define MHTRI_MENU_EQUIP_DETAIL_SCREEN_RESET_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8030BACC - menu page 8's opener (list 1 row 2): resets the status screen's counters for the equip-detail mode 8 */
void equip_detail_screen_reset(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EQUIP_DETAIL_SCREEN_RESET_H */
