/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_friendlist` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MOVIE_PAGE04_SPLIT_OPEN_H
#define MHTRI_MENU_MOVIE_PAGE04_SPLIT_OPEN_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80442294 - the split-screen menu's page 4 opener (its top menu row 3) */
void movie_page04_split_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MOVIE_PAGE04_SPLIT_OPEN_H */
