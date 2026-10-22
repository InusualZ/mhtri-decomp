/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/fn_80212810.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_ITEM_DETAIL_PANEL_DRAW_H
#define MHTRI_LOBBY_LB_ITEM_DETAIL_PANEL_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80217058 - draws item `item`'s detail panel in layout panel `panel`. */
void lb_item_detail_panel_draw(u16 panel, u16 item, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_DETAIL_PANEL_DRAW_H */
