/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/movie` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MOVIE_PAGE17_OPEN_H
#define MHTRI_MENU_MOVIE_PAGE17_OPEN_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x804429DC - menu page 17's opener (sub-list kind 17) */
void movie_page17_open(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MOVIE_PAGE17_OPEN_H */
