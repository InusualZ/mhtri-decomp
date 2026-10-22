/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_infomation` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C++ linkage (the map row is this declaration's mangling, rule 9). */
#ifndef MHTRI_MENU_SET_EQUIP_COLUMN_ARRANGEMENT_H
#define MHTRI_MENU_SET_EQUIP_COLUMN_ARRANGEMENT_H

#include "types.h"

struct _PLW;
struct _EQUIP_INDEX;
struct _EQUIP;

/* 0x8031A244 - fills the equipment record the status pages read for a player */
u8 Set_equip_column_arrangement(_PLW* plw, _EQUIP_INDEX* index, _EQUIP* out);

#endif /* MHTRI_MENU_SET_EQUIP_COLUMN_ARRANGEMENT_H */
