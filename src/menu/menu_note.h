/*
 * `menu/menu_note.cpp`'s own view: the note-page slot lookup the item page and this unit call.
 *
 * RULE 2 HOME.  `get_note_item_slot` is defined by `src/menu/menu_note.cpp` (map `.text` 0x8034C0C4)
 * and had no header, so the declaration lived nowhere: this header is its home.  The signature is the
 * owner's (`u8 note_entry, u8* slot_tbl, u8* slot_idx` - the callee writes the two bytes through the
 * pointers and never reads them), copied from its definition, not from a call site.
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
