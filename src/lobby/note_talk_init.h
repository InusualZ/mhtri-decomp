/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_note_talk.cpp`'s talk NPC record init and frame, which
 * `enemy/em_prog_support.cpp` runs for its kind-0 records.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_NOTE_TALK_INIT_H
#define MHTRI_LOBBY_NOTE_TALK_INIT_H

#include "types.h"

struct NoteWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8038EF28 - places the talk NPC at its spot and starts it idling. */
void note_talk_init(struct NoteWork* self);
/* 0x8038EF9C - the talk NPC's frame. */
void note_talk_frame(struct NoteWork* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_NOTE_TALK_INIT_H */
