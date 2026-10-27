/* `menu_choice_window_draw`, defined by `menu/menu_message.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_CHOICE_WINDOW_DRAW_H
#define MHTRI_MENU_MENU_CHOICE_WINDOW_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802AA0DC (0x1C0): draws a choice window of count entries at pos. */
struct MenuEntry;
struct _mh_ivec2_;
void menu_choice_window_draw(struct MenuEntry* entries, u8 count, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_CHOICE_WINDOW_DRAW_H */
