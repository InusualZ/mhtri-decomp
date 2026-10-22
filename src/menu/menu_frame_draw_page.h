/* `menu_frame_draw_page`, defined by `menu/menu_message.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_FRAME_DRAW_PAGE_H
#define MHTRI_MENU_MENU_FRAME_DRAW_PAGE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A6EF4 (0x70): draws one page of an item grid (the slot table, its cells, the page, the page count, the fade, the flags). */
struct IdValue;
struct MenuEntry;
void menu_frame_draw_page(struct IdValue* page, struct MenuEntry* cells, s8 page_no, s8 pages, u16 fade, s32 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_FRAME_DRAW_PAGE_H */
