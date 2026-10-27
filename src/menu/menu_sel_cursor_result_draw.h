/* `menu_sel_cursor_result_draw`, defined by `menu/menu_combo.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_SEL_CURSOR_RESULT_DRAW_H
#define MHTRI_MENU_MENU_SEL_CURSOR_RESULT_DRAW_H

#include "types.h"

struct MenuSelCursor;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the combination result row with the row table `rows`. */
void menu_sel_cursor_result_draw(struct MenuSelCursor* cur, const u16* rows);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SEL_CURSOR_RESULT_DRAW_H */
