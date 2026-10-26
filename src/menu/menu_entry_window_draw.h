/* menu/menu_entry_window_draw.h - leaf header (docs/plan.md 6.5 rule 2) for `menu/menu_message.cpp`'s entry window
 *   draw.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_ENTRY_WINDOW_DRAW_H
#define MHTRI_MENU_MENU_ENTRY_WINDOW_DRAW_H

#include "types.h"

struct MenuEntry;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802AA29C (0x150): draws window `id` and one row per entry of `rows`, with the cursor on the selected one. */
void menu_entry_window_draw(u16 id, u32* frame_ids, u32* cursor_ids, struct MenuEntry* rows, u8 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ENTRY_WINDOW_DRAW_H */
