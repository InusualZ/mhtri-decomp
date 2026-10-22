/* Leaf header (docs/plan.md 6.5 rule 2): the `ef/fn_8030681C` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_EF_MENU_STATUS_PAGE_STEP_H
#define MHTRI_EF_MENU_STATUS_PAGE_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80308F1C - menu page 7's frame handler (page dispatcher, kind 7) */
s32 menu_status_page_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_MENU_STATUS_PAGE_STEP_H */
