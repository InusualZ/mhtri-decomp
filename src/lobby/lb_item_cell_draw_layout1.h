/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/fn_80212810.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT1_H
#define MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT1_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80216560 - draws an item cell with layout table 1. */
s32 lb_item_cell_draw_layout1(u16 a, u16 b, s16* c, s32 d, s32 e, s32 f, s32 g);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT1_H */
