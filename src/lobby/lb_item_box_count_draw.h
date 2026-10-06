/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_equip_page.cpp`'s item-box count draw.  C linkage. */
#ifndef MHTRI_LOBBY_LB_ITEM_BOX_COUNT_DRAW_H
#define MHTRI_LOBBY_LB_ITEM_BOX_COUNT_DRAW_H

#include "types.h"

struct Eft052ItemBox;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80222BC4 - draws the count of the item box's cursor cell at panel `panel` (GUESS name). */
void lb_item_box_count_draw(struct Eft052ItemBox* box, u16 panel, u8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_BOX_COUNT_DRAW_H */
