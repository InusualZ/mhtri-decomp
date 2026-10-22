/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C++ linkage (the map row is this declaration's mangling, rule 9). */
#ifndef MHTRI_MENU_PUT_STATUS_EQUIP_ELEMENT_BLOCK_H
#define MHTRI_MENU_PUT_STATUS_EQUIP_ELEMENT_BLOCK_H

#include "types.h"

struct _PLW;
struct _EQUIP;
struct _mh_ivec2_;

/* 0x80309A90 - draws the status page's element block */
void Put_status_equip_element_block(_PLW* plw, _EQUIP* equip, u16 flags, _mh_ivec2_* pos);

#endif /* MHTRI_MENU_PUT_STATUS_EQUIP_ELEMENT_BLOCK_H */
