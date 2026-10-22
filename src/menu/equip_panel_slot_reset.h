/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_EQUIP_PANEL_SLOT_RESET_H
#define MHTRI_MENU_EQUIP_PANEL_SLOT_RESET_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031A58C - menu page 1's opener (the top menu's list 0 row 1): resets the slot into its equipment-panel state; the menu work dispatcher calls it */
void equip_panel_slot_reset(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EQUIP_PANEL_SLOT_RESET_H */
