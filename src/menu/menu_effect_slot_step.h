/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_effect_slot` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_EFFECT_SLOT_STEP_H
#define MHTRI_MENU_MENU_EFFECT_SLOT_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80348A78 - menu page 3's frame handler (page dispatcher, kind 3; menu_effect_slot_open opens it) */
s32 menu_effect_slot_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_EFFECT_SLOT_STEP_H */
