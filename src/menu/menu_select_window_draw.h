/* menu/menu_select_window_draw.h - leaf header: `menu_select_window_draw` (0x802A9F48), owned by `menu/menu_message.cpp`. */
#ifndef MHTRI_MENU_MENU_SELECT_WINDOW_DRAW_H
#define MHTRI_MENU_MENU_SELECT_WINDOW_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif
/* Draws the selection window `kind` with the message `text` at `pos` shifted by `offset`, row `value` highlighted. */
void menu_select_window_draw(u8 value, u16 kind, s8* text, const struct _mh_ivec2_* pos, const struct _mh_ivec2_* offset);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SELECT_WINDOW_DRAW_H */
