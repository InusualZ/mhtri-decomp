/*
 * `note_work_find` (0x80385698), defined by `enemy/em_prog_support.cpp` (leaf header, rule 2): the live note-pane
 * record of a kind, or NULL.
 */
#ifndef MHTRI_ENEMY_NOTE_WORK_FIND_H
#define MHTRI_ENEMY_NOTE_WORK_FIND_H

#include "types.h"
#include "enemy/note_work.h"

#ifdef __cplusplus
extern "C" {
#endif

NoteWork* note_work_find(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_NOTE_WORK_FIND_H */
