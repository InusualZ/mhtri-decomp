/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s `lb_price_cell_draw`, the one declaration `lobby/lb_npc.cpp` needs.
 * C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_LOBBY_LB_PRICE_CELL_DRAW_H
#define MHTRI_LOBBY_LB_PRICE_CELL_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80216598 - draws a price cell through lb_item_cell_draw_layout */
void lb_price_cell_draw(u16 item, s32 price, struct _mh_ivec2_* pos, s32 enabled, s32 picked, s32 active, u16 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PRICE_CELL_DRAW_H */
