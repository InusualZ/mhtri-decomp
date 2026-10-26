/* Leaf header (docs/plan.md 6.5 rule 2): `menu_area_fx_set`, which `menu/menu_item_effect` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_MENU_MENU_AREA_FX_SET_H
#define MHTRI_MENU_MENU_AREA_FX_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8031DAA8 - starts the menu-band area effect. */
void menu_area_fx_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_AREA_FX_SET_H */
