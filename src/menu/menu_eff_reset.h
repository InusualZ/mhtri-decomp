/* Leaf header (docs/plan.md 6.5 rule 2): `menu_eff_reset`, which `menu/fn_8031EA8C` owns; declared for
 * `menu/menu_item_effect.cpp`.  C linkage. */
#ifndef MHTRI_MENU_MENU_EFF_RESET_H
#define MHTRI_MENU_MENU_EFF_RESET_H

#include "types.h"
#include "ef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031EA8C - restarts the effect's pop-in and shows the model `mode` selects. */
void menu_eff_reset(_EFT* self, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_EFF_RESET_H */
