/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_message` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_FRAME_PAGE_DRAW_H
#define MHTRI_MENU_MENU_FRAME_PAGE_DRAW_H

#include "types.h"

struct IdValue;
struct MenuEntry;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A736C - draws one page of the item grid */
void menu_frame_page_draw(IdValue* page, MenuEntry* cells, s8 cols, s8 rows, u16 timer, u32 flags, u8 player);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_FRAME_PAGE_DRAW_H */
