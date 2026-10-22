/* `menu_cursor_column_step`, defined by `menu/menu_message.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_CURSOR_COLUMN_STEP_H
#define MHTRI_MENU_MENU_CURSOR_COLUMN_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A6C1C (0xC): steps the list cursor one column. */
struct MenuListWork;
void menu_cursor_column_step(struct MenuListWork* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_CURSOR_COLUMN_STEP_H */
