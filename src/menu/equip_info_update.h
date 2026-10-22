/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_EQUIP_INFO_UPDATE_H
#define MHTRI_MENU_EQUIP_INFO_UPDATE_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80308FB4 - draws the equipment info page (menu page 7) */
void equip_info_update(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EQUIP_INFO_UPDATE_H */
