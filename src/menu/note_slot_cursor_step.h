/* Leaf header (docs/plan.md 6.5 rule 2): symbols `menu/menu_row.cpp` defines, for a consumer that does not include
 * the owner's header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_NOTE_SLOT_CURSOR_STEP_H
#define MHTRI_MENU_NOTE_SLOT_CURSOR_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034C1D0 - steps the note list's cursor `*cursor`/`*word` by the pad bits `flags`; 1 while the list stays open. */
s32 note_slot_cursor_step(u8 note_entry, u8* cursor, u16 flags, u16* word);
/* 0x8034C2B8 - draws the note list's rows with the cursor row `light`. */
void note_slot_draw_row(u8 note_entry, s8 light, u16 word);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_NOTE_SLOT_CURSOR_STEP_H */
