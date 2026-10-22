/* `lb_pane_draw` and `lb_entry_pane_sell_step`, defined by `lobby/lb_pane_ui.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_PANE_DRAW_H
#define MHTRI_LOBBY_LB_PANE_DRAW_H

#include "types.h"

struct LbEntryPane;
struct LbBoxMenu;
struct LbEquipBoxWork;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the equipment box pane `pane` at (`x`, `y`) for owner `owner`. */
void lb_pane_draw(struct LbEntryPane* pane, s16 x, s16 y, struct LbBoxMenu* owner);
/* One input step of the box pane's sell screen: 2 when it is left. */
s32 lb_entry_pane_sell_step(struct LbEntryPane* pane, struct LbEquipBoxWork* owner);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PANE_DRAW_H */
