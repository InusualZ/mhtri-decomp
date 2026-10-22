/* `equip_list_draw`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_EQUIP_LIST_DRAW_H
#define MHTRI_MENU_EQUIP_LIST_DRAW_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the rows of a nine-record equipment set at layout sprite `lsp`, the `cursor` row lit. */
void equip_list_draw(struct _EQUIP* recs, s8 cursor, u8 dye, u16 lsp, u16 present);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EQUIP_LIST_DRAW_H */
