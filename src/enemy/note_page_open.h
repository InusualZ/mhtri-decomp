/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em_prog_support` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_NOTE_PAGE_OPEN_H
#define MHTRI_ENEMY_NOTE_PAGE_OPEN_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x803843D8 - menu page 25's opener (list 2 row 1, kind 0x19): seeds the note pane */
void note_page_open(MenuSlot* slot, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NOTE_PAGE_OPEN_H */
