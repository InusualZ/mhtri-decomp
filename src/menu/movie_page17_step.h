/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/movie` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MOVIE_PAGE17_STEP_H
#define MHTRI_MENU_MOVIE_PAGE17_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80442D60 - menu page 17's frame handler (page dispatcher, kind 17) */
s32 movie_page17_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MOVIE_PAGE17_STEP_H */
