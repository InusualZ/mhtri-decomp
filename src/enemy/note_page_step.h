/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em_prog_support` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_NOTE_PAGE_STEP_H
#define MHTRI_ENEMY_NOTE_PAGE_STEP_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80384434 - menu page 25's frame handler (page dispatcher, kind 25) */
s32 note_page_step(MenuSlot* slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NOTE_PAGE_STEP_H */
