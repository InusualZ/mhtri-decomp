/* The enemy note-pane band's program tail `enemy/em_prog_tail.cpp` (`.text` 0x80385EE0..0x803868DC): the entry its
 * neighbour `enemy/em_prog_support.cpp` tail-calls (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_ENEMY_EM_PROG_TAIL_H
#define MHTRI_ENEMY_EM_PROG_TAIL_H

#include "types.h"
#include "enemy/note_work.h"

extern "C" {

/* 0x803865B4 - the motion-keyed effect cues `enemy/em_prog_support.cpp` hands over to (the last reader of the note
 * records). */
void note_pane_motion_effects(NoteWork* self);
/* 0x80385EF8 - the lobby NPC record's frame. */
void note_pane_npc_frame(NoteWork* self);

}


#endif /* MHTRI_ENEMY_EM_PROG_TAIL_H */
