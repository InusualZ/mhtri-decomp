/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/multi_result.cpp` symbols `lobby/lb_quest_screen.cpp`'s note pane
 * uses.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_NOTE_PANE_PLAYER_NEAR_CK_H
#define MHTRI_MENU_NOTE_PANE_PLAYER_NEAR_CK_H

#include "types.h"

struct NoteWork;
struct Vec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803A357C - whether the local player is up, in the pane's area, within 500 units and ready to talk (GUESS name). */
u32 note_pane_player_near_ck(struct NoteWork* self);

/* `.data` 0x805F1E78 - the note pane's home spot per map kind (GUESS name). */
extern struct Vec note_pane_home_table[6];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_NOTE_PANE_PLAYER_NEAR_CK_H */
