/* `put_equip_panel_row`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_PUT_EQUIP_PANEL_ROW_H
#define MHTRI_MENU_PUT_EQUIP_PANEL_ROW_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Puts the bowgun set's panel row (second page) at layout sprite `lsp`. */
void put_equip_panel_row(struct _PLW* plw, struct _EQUIP* gun, struct _EQUIP* opt_a, struct _EQUIP* opt_b, u16 lsp, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PUT_EQUIP_PANEL_ROW_H */
