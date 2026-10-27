/* `menu_item_cell_draw`, defined by `menu/menu_combo.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_ITEM_CELL_DRAW_H
#define MHTRI_MENU_MENU_ITEM_CELL_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws one item cell: icon, name and count, lit when active and dimmed on request. */
void menu_item_cell_draw(struct _mh_ivec2_* pos, u16 id, s16 count, s32 has, s32 active, s32 dim);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ITEM_CELL_DRAW_H */
