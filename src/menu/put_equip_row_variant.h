/* `put_equip_row_variant`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_PUT_EQUIP_ROW_VARIANT_H
#define MHTRI_MENU_PUT_EQUIP_ROW_VARIANT_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Puts the bowgun set's rows (first page), its variants and the panel row at layout sprite `lsp`. */
u8 put_equip_row_variant(struct _PLW* plw, struct _EQUIP* gun, struct _EQUIP* opt_a, struct _EQUIP* opt_b, u16 lsp, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PUT_EQUIP_ROW_VARIANT_H */
