/* Leaf header (docs/plan.md 6.5 rule 2): the `enemy/em_prog_support` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_NOTE_PANE_RESET_H
#define MHTRI_ENEMY_NOTE_PANE_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80384380 - clears the note pane's +0x07/+0x09 bytes, sets +0x08 to 4 and copies +0x04 into +0x06; called when the menu closes */
void note_pane_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NOTE_PANE_RESET_H */
