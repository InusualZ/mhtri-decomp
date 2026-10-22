/* `lb_equip_page_draw`, defined by `lobby/fn_80219260.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_PAGE_DRAW_H
#define MHTRI_LOBBY_LB_EQUIP_PAGE_DRAW_H

#include "types.h"

struct LbScrollWork;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the equipment page of the scroll list (`mode` 1 with the cursor). */
void lb_equip_page_draw(struct LbScrollWork* scroll, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_PAGE_DRAW_H */
