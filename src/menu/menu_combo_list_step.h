/* menu/menu_combo_list_step.h - leaf header: `menu_combo_list_step` (0x80349228), owned by `menu/menu_effect_slot.cpp`. */
#ifndef MHTRI_MENU_MENU_COMBO_LIST_STEP_H
#define MHTRI_MENU_MENU_COMBO_LIST_STEP_H

#include "types.h"

struct MenuSlot;

#ifdef __cplusplus
extern "C" {
#endif
/* Steps the recipe list on `slot`; returns 2 when it closes. */
s32 menu_combo_list_step(struct MenuSlot* slot);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_COMBO_LIST_STEP_H */
