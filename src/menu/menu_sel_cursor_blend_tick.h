/* `menu_sel_cursor_blend_tick`, defined by menu/fn_8031A6C0.cpp (leaf header, rule 2). */
#ifndef MHTRI_MENU_MENU_SEL_CURSOR_BLEND_TICK_H
#define MHTRI_MENU_MENU_SEL_CURSOR_BLEND_TICK_H

#include "types.h"

struct MenuSelCursor;

#ifdef __cplusplus
extern "C" {
#endif

/* Steps the cursor's two blend counters for one frame. */
void menu_sel_cursor_blend_tick(struct MenuSelCursor* cur);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_SEL_CURSOR_BLEND_TICK_H */
