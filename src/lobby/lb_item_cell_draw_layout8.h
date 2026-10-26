/* `lb_item_cell_draw_layout8`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT8_H
#define MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT8_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802166B0 - draws an item cell in layout 8. */
void lb_item_cell_draw_layout8(u16 item, struct _mh_ivec2_* pos, bool enabled, bool sel, bool active, u16 flags);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_CELL_DRAW_LAYOUT8_H */
