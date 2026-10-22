/* `lb_equip_cell_draw`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_H
#define MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws an equipment cell at layout sprite `lsp`: rare colour, icon, name and sell price. */
void lb_equip_cell_draw(u16 lsp, struct _EQUIP* rec, u8 mode, s16 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_CELL_DRAW_H */
