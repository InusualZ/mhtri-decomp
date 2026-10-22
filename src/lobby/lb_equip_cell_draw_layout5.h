/* `lb_equip_cell_draw_layout5` and `lb_equip_state_get`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_LAYOUT5_H
#define MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_LAYOUT5_H

#include "types.h"

struct _EQUIP;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws equipment cell `equip` at `pos` with its price and its open, picked, active and kind-flag looks. */
void lb_equip_cell_draw_layout5(struct _EQUIP* equip, u32 price, struct _mh_ivec2_* pos, s32 open, s32 sel, s32 active,
                                u8 flags, s32 mode);
/* The state of armour piece `equip` (0: not yet owned by the player's set). */
s32 lb_equip_state_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_LAYOUT5_H */
