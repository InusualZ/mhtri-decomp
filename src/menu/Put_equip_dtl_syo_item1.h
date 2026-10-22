/* `Put_equip_dtl_syo_item1`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2).  C++ linkage: the map
 * spells it `Put_equip_dtl_syo_item1__FP6_EQUIPP6_EQUIPbbUc10_mh_ivec2_Us`; the position is passed by value. */
#ifndef MHTRI_MENU_PUT_EQUIP_DTL_SYO_ITEM1_H
#define MHTRI_MENU_PUT_EQUIP_DTL_SYO_ITEM1_H

#include "types.h"

struct _EQUIP;
struct _mh_ivec2_;

/* Draws one equipment detail row (name, icon and dye mark) of `equip` at layout sprite `lsp` from `pos`. */
void Put_equip_dtl_syo_item1(struct _EQUIP* equip, struct _EQUIP* other, bool active, bool locked, u8 dye,
                             struct _mh_ivec2_ pos, u16 lsp);

#endif /* MHTRI_MENU_PUT_EQUIP_DTL_SYO_ITEM1_H */
