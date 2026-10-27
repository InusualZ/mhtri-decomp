/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_friendlist` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MOVIE_PAGE04_SPLIT_STEP_H
#define MHTRI_MENU_MOVIE_PAGE04_SPLIT_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044256C - the split-screen menu's page 4 frame handler */
s32 movie_page04_split_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MOVIE_PAGE04_SPLIT_STEP_H */
