/* `lb_equip_cell_draw_layout3`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_LAYOUT3_H
#define MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_LAYOUT3_H

#include "types.h"

struct _EQUIP;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws an equipment cell with layout 3 (`lb_equip_cell_draw_layout`). */
void lb_equip_cell_draw_layout3(struct _EQUIP* rec, s32 b, struct _mh_ivec2_* pos, u32 lit, s32 picked, s32 f, u8 flags, s32 h);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_LAYOUT3_H */
