/*
 * menu/menu_note.h - `get_note_item_slot`, defined by `menu/menu_note.cpp` (0x8034C0C4): the owner's signature, which
 *   writes the two bytes through the pointers and never reads them.
 */
#ifndef MHTRI_MENU_MENU_NOTE_H
#define MHTRI_MENU_MENU_NOTE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void get_note_item_slot(u8 note_entry, u8* slot_tbl, u8* slot_idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_NOTE_H */
