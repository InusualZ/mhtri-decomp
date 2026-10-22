/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em_prog_support` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_NOTE_PEN_SEED_H
#define MHTRI_ENEMY_NOTE_PEN_SEED_H

#include "types.h"

struct MenuSlot;
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80384324 - seeds the note pen's layout state from the screen frame scale and rebuilds it (the comment on the owner's body); the menu calls it after opening the keyboard */
void note_pen_seed(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NOTE_PEN_SEED_H */
