/* `lb_count_window_draw`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_COUNT_WINDOW_DRAW_H
#define MHTRI_LOBBY_LB_COUNT_WINDOW_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the count window of panel `panel`/`sub_panel`: `value` of `max` at `price` each. */
void lb_count_window_draw(s32 panel, s32 sub_panel, s32 value, s32 max, s32 price, s32 flag, s32 moved);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COUNT_WINDOW_DRAW_H */
