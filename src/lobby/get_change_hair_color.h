/* lobby/get_change_hair_color.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_pane_ui.cpp`'s palette colours, for `menu/movie.cpp`.  C++ scope (the map rows are `get_change_hair_color__FUcPUlPUsPsPs` and its inner-wear twin). */
#ifndef MHTRI_LOBBY_GET_CHANGE_HAIR_COLOR_H
#define MHTRI_LOBBY_GET_CHANGE_HAIR_COLOR_H

#include "types.h"

/* 0x801FB1D8 / 0x801FB240 - palette cell `index`'s hair / inner-wear colour, its id and its box position. */
void get_change_hair_color(u8 index, u32* color, u16* out_id, s16* out_x, s16* out_y);
void get_change_inner_color(u8 index, u32* color, u16* out_id, s16* out_x, s16* out_y);

#endif /* MHTRI_LOBBY_GET_CHANGE_HAIR_COLOR_H */
