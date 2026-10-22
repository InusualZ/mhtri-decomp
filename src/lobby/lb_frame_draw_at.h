/* `lb_frame_draw_at`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_FRAME_DRAW_AT_H
#define MHTRI_LOBBY_LB_FRAME_DRAW_AT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws a list frame with text `text` at panel `sub_panel` inside panel `panel`. */
void lb_frame_draw_at(u16 panel, u16 sub_panel, s8* text, s32 sel, u32 color, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_FRAME_DRAW_AT_H */
