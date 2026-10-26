/* The menu band `menu/menu_effect_slot.cpp` (`.text` 0x80348A48..0x80349DD8): the entry the item selection screen
 * calls (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_MENU_MENU_EFFECT_SLOT_H
#define MHTRI_MENU_MENU_EFFECT_SLOT_H

#include "types.h"

extern "C" {

/* 0x80349184 - the selection screen's call over one slot record (no body yet). */
void menu_combo_list_open(void* a);

}

#endif /* MHTRI_MENU_MENU_EFFECT_SLOT_H */
