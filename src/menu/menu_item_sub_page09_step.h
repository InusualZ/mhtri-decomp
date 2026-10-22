/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_item_sub` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_ITEM_SUB_PAGE09_STEP_H
#define MHTRI_MENU_MENU_ITEM_SUB_PAGE09_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8036C284 - menu page 9's frame handler (page dispatcher, kind 9) */
s32 menu_item_sub_page09_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ITEM_SUB_PAGE09_STEP_H */
