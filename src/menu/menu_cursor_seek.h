/* menu/menu_cursor_seek.h - leaf header (docs/plan.md 6.5 rule 2) for `menu/menu_message.cpp`'s cursor seek.
 *   C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_MENU_CURSOR_SEEK_H
#define MHTRI_MENU_MENU_CURSOR_SEEK_H

#include "types.h"

struct MenuListWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A6B6C (0xB0): moves the list cursor to flat entry `index`. */
void menu_cursor_seek(struct MenuListWork* self, s8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_CURSOR_SEEK_H */
