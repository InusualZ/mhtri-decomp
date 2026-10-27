/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_prog_tail.cpp`'s lobby NPC record init, which
 * `enemy/em_prog_support.cpp` runs for its kind-2 records.  C linkage (the map row is a plain name). */
#ifndef MHTRI_ENEMY_NOTE_PANE_NPC_INIT_H
#define MHTRI_ENEMY_NOTE_PANE_NPC_INIT_H

#include "types.h"

struct NoteWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80385EEC - clears the record's active flag and returns it to animation pair (0, 0). */
void note_pane_npc_init(struct NoteWork* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NOTE_PANE_NPC_INIT_H */
