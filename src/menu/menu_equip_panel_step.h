/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_combo` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_EQUIP_PANEL_STEP_H
#define MHTRI_MENU_MENU_EQUIP_PANEL_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031A6C0 - menu page 1's frame handler, called through the page dispatcher's jump table for kind 1; returns the dispatcher's 0-4 result */
s32 menu_equip_panel_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_EQUIP_PANEL_STEP_H */
