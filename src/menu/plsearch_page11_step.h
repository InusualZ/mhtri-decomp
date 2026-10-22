/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_plsearch` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_PLSEARCH_PAGE11_STEP_H
#define MHTRI_MENU_PLSEARCH_PAGE11_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044F7FC - menu page 11's frame handler (page dispatcher, kind 11) */
s32 plsearch_page11_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLSEARCH_PAGE11_STEP_H */
