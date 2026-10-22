/* `place_list_highlight_draw`, defined by `menu/menu_row.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_PLACE_LIST_HIGHLIGHT_DRAW_H
#define MHTRI_MENU_PLACE_LIST_HIGHLIGHT_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034CDDC (0x348): draws the place-list boxes, highlighting entry *index. */
void place_list_highlight_draw(u8 mode, const u32* index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACE_LIST_HIGHLIGHT_DRAW_H */
