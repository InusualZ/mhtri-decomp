/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_effect_slot` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_EFFECT_SLOT_OPEN_H
#define MHTRI_MENU_MENU_EFFECT_SLOT_OPEN_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80348A48 - puts a menu slot into sub-page 3 (+0x14 = 3) and resets its cursors; the top menu's effect-slot row and shortcut */
void menu_effect_slot_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_EFFECT_SLOT_OPEN_H */
