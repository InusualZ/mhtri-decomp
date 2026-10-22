/* `Put_status_equip_status_block` and `Put_status_equip_element_block`, defined by `menu/menu_infomation.cpp`
 * (leaf header, rule 2).  C++ linkage: the map spells them `..__FP4_PLWP6_EQUIPUsP10_mh_ivec2_`. */
#ifndef MHTRI_MENU_PUT_STATUS_EQUIP_STATUS_BLOCK_H
#define MHTRI_MENU_PUT_STATUS_EQUIP_STATUS_BLOCK_H

#include "types.h"

struct _PLW;
struct _EQUIP;
struct _mh_ivec2_;

/* Draws the attack/defence status block of the equipment records at layout sprite `lsp`. */
void Put_status_equip_status_block(struct _PLW* plw, struct _EQUIP* recs, u16 lsp, struct _mh_ivec2_* pos);

/* Draws the element block of the equipment records at layout sprite `lsp`. */
void Put_status_equip_element_block(struct _PLW* plw, struct _EQUIP* recs, u16 lsp, struct _mh_ivec2_* pos);

#endif /* MHTRI_MENU_PUT_STATUS_EQUIP_STATUS_BLOCK_H */
