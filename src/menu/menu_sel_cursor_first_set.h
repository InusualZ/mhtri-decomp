/* `menu_sel_cursor_first_set`, defined by menu/menu_combo.cpp (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_SEL_CURSOR_FIRST_SET_H
#define MHTRI_MENU_MENU_SEL_CURSOR_FIRST_SET_H

#include "types.h"

struct MenuSelCursor;

#ifdef __cplusplus
extern "C" {
#endif

/* Stores the pair's first slot and its item id. */
void menu_sel_cursor_first_set(struct MenuSelCursor* cur, u16 slot, u16 item_id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SEL_CURSOR_FIRST_SET_H */
