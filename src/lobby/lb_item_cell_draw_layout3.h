/* `lb_item_cell_draw_layout3`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT3_H
#define MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT3_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the lobby item cell of `item` at `pos` with layout 3. */
void lb_item_cell_draw_layout3(u16 item, s32 count, struct _mh_ivec2_* pos, bool enabled, bool sel, bool active,
                               u16 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT3_H */
