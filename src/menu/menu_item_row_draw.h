/* `menu_item_row_draw` and `menu_item_row_draw_pages`, defined by `menu/menu_message.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_ITEM_ROW_DRAW_H
#define MHTRI_MENU_MENU_ITEM_ROW_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A7838 - draws the item row of `item` at `pos`. */
void menu_item_row_draw(u16* item, struct _mh_ivec2_* pos);
/* 0x802A7B80 - the same row with the page counts' values. */
void menu_item_row_draw_pages(u16* item, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ITEM_ROW_DRAW_H */
