/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_placeinfo` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_PLACEINFO_PAGE12_STEP_H
#define MHTRI_MENU_PLACEINFO_PAGE12_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043B614 - menu page 12's frame handler (page dispatcher, kind 12) */
s32 placeinfo_page12_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACEINFO_PAGE12_STEP_H */
