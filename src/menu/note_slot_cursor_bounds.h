/* `note_slot_cursor_bounds`, defined by `menu/menu_row.cpp` (leaf header, rule 2; `menu/menu_row.h` does not declare
 * it).  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_NOTE_SLOT_CURSOR_BOUNDS_H
#define MHTRI_MENU_NOTE_SLOT_CURSOR_BOUNDS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034C614 - the (first row, row count) pair of entry `index` of note list `kind`, in the result's high half. */
u32 note_slot_cursor_bounds(u8 kind, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_NOTE_SLOT_CURSOR_BOUNDS_H */
