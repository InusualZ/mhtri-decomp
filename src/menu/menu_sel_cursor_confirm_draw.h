/* `menu_sel_cursor_confirm_draw`, defined by `menu/fn_8031A6C0.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_SEL_CURSOR_CONFIRM_DRAW_H
#define MHTRI_MENU_MENU_SEL_CURSOR_CONFIRM_DRAW_H

#include "types.h"

struct MenuSelCursor;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the combination's confirm question with `sel` picked at layout sprite `lsp`. */
void menu_sel_cursor_confirm_draw(struct MenuSelCursor* cur, s32 sel, u16 lsp);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SEL_CURSOR_CONFIRM_DRAW_H */
